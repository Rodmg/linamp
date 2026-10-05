#include "systemequalizer.h"
#include "equalizerbands.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

#include <cerrno>
#include <cmath>
#include <cstring>
#include <functional>

#include <pipewire/pipewire.h>
#include <pipewire/thread-loop.h>
#include <pipewire/extensions/metadata.h>
#include <pipewire/impl-module.h>
#include <spa/param/props.h>
#include <spa/pod/builder.h>
#include <spa/utils/string.h>

namespace {

const char *kEqNodeName = "linamp-eq";
const char *kEqOutputName = "linamp-eq-output";
const char *kDefaultSinkKey = "default.audio.sink";
const char *kConfiguredSinkKey = "default.configured.audio.sink";
constexpr int kPwTimeoutMs = 3000;

QString sinkNameFromMetadata(const char *value)
{
    if (value == nullptr || value[0] == '\0') {
        return {};
    }
    if (value[0] == '{') {
        const QJsonDocument doc = QJsonDocument::fromJson(QByteArray(value));
        return doc.object().value("name").toString();
    }
    return QString::fromUtf8(value);
}

QString bandGainKey(int index)
{
    return QString("b%1:Gain").arg(index);
}

int clampUi(int value, int min, int max)
{
    return value < min ? min : (value > max ? max : value);
}

float uiToLinear(int value)
{
    const float t = clampUi(value, 0, 100) / 100.0f;
    return t * t * t;
}

void volumeToChannels(int volume, int balance, float *left, float *right)
{
    const float peak = uiToLinear(volume);
    if (balance <= 0) {
        *left = peak;
        *right = peak * (100 + clampUi(balance, -100, 100)) / 100.0f;
    } else {
        *right = peak;
        *left = peak * (100 - clampUi(balance, -100, 100)) / 100.0f;
    }
}

}

class SystemEqualizer::Pw {
public:
    explicit Pw(SystemEqualizer *owner)
        : owner(owner)
    {
    }

    SystemEqualizer *owner = nullptr;

    pw_thread_loop *loop = nullptr;
    pw_context *context = nullptr;
    pw_core *core = nullptr;
    pw_registry *registry = nullptr;
    pw_metadata *metadata = nullptr;
    // Volume uses its own client. The filter-chain client must not also
    // proxy the hardware sink; doing that wedged PipeWire once audio started.
    pw_core *volumeCore = nullptr;
    pw_registry *volumeRegistry = nullptr;

    pw_node *node = nullptr;
    pw_node *outputNode = nullptr;
    pw_node *hardwareNode = nullptr;
    pw_impl_module *module = nullptr;

    spa_hook coreListener = {};
    spa_hook registryListener = {};
    spa_hook metadataListener = {};
    spa_hook volumeCoreListener = {};

    bool loopStarted = false;
    bool coreListenerAdded = false;
    bool registryListenerAdded = false;
    bool metadataListenerAdded = false;
    bool volumeCoreListenerAdded = false;
    bool pwInit = false;

    bool syncDone = false;
    int syncSeq = 0;
    bool volumeSyncDone = false;
    int volumeSyncSeq = 0;
    uint32_t eqNodeId = 0;
    uint32_t eqOutputNodeId = 0;
    uint32_t hardwareNodeId = 0;

    QString defaultSinkName;
    QString hardwareSink;
    QStringList sinkNames;
    QHash<QString, uint32_t> sinkIds;
    bool hardwareSinkChosen = false;
    bool claimedDefault = false;

    static void onCoreDone(void *data, uint32_t id, int seq)
    {
        auto *pw = static_cast<Pw *>(data);
        if (id == PW_ID_CORE && seq == pw->syncSeq) {
            pw->syncDone = true;
        }
        if (pw->loop != nullptr) {
            pw_thread_loop_signal(pw->loop, false);
        }
    }

    static void onCoreError(void *data, uint32_t id, int seq, int res, const char *message)
    {
        Q_UNUSED(data);
        Q_UNUSED(seq);
        qWarning() << "PipeWire EQ error id" << id << "res" << res << (message != nullptr ? message : "");
    }

    static void onVolumeCoreDone(void *data, uint32_t id, int seq)
    {
        auto *pw = static_cast<Pw *>(data);
        if (id == PW_ID_CORE && seq == pw->volumeSyncSeq) {
            pw->volumeSyncDone = true;
        }
        if (pw->loop != nullptr) {
            pw_thread_loop_signal(pw->loop, false);
        }
    }

    static int onMetadataProperty(void *data, uint32_t subject, const char *key, const char *type, const char *value)
    {
        Q_UNUSED(type);
        auto *pw = static_cast<Pw *>(data);
        if (subject != PW_ID_CORE || key == nullptr || pw->hardwareSinkChosen) {
            return 0;
        }
        if (!spa_streq(key, kDefaultSinkKey)) {
            return 0;
        }
        const QString name = sinkNameFromMetadata(value);
        if (!name.isEmpty() && name != QLatin1String(kEqNodeName)) {
            pw->defaultSinkName = name;
        }
        return 0;
    }

    static void onGlobal(void *data, uint32_t id, uint32_t permissions, const char *type, uint32_t version, const struct spa_dict *props)
    {
        Q_UNUSED(permissions);
        Q_UNUSED(version);
        auto *pw = static_cast<Pw *>(data);
        if (type == nullptr) {
            return;
        }

        if (spa_streq(type, PW_TYPE_INTERFACE_Metadata) && pw->metadata == nullptr) {
            const char *name = props != nullptr ? spa_dict_lookup(props, PW_KEY_METADATA_NAME) : nullptr;
            if (name != nullptr && spa_streq(name, "default")) {
                pw->metadata = static_cast<pw_metadata *>(pw_registry_bind(
                    pw->registry, id, PW_TYPE_INTERFACE_Metadata, PW_VERSION_METADATA, 0));
                if (pw->metadata != nullptr) {
                    static const pw_metadata_events events = {
                        .version = PW_VERSION_METADATA_EVENTS,
                        .property = &Pw::onMetadataProperty,
                    };
                    pw_metadata_add_listener(pw->metadata, &pw->metadataListener, &events, pw);
                    pw->metadataListenerAdded = true;
                }
            }
        }

        if (spa_streq(type, PW_TYPE_INTERFACE_Node) && props != nullptr) {
            const char *name = spa_dict_lookup(props, PW_KEY_NODE_NAME);
            const char *mediaClass = spa_dict_lookup(props, PW_KEY_MEDIA_CLASS);
            if (name != nullptr && spa_streq(name, kEqNodeName)) {
                pw->eqNodeId = id;
                pw_thread_loop_signal(pw->loop, false);
            }
            if (name != nullptr && spa_streq(name, kEqOutputName)) {
                pw->eqOutputNodeId = id;
                pw_thread_loop_signal(pw->loop, false);
            }
            if (name != nullptr && mediaClass != nullptr && spa_streq(mediaClass, "Audio/Sink")
                && !spa_streq(name, kEqNodeName)) {
                const QString sinkName = QString::fromUtf8(name);
                pw->sinkIds.insert(sinkName, id);
                if (!pw->sinkNames.contains(sinkName)) {
                    pw->sinkNames.append(sinkName);
                }
            }
        }
    }

    static void onGlobalRemove(void *data, uint32_t id)
    {
        auto *pw = static_cast<Pw *>(data);
        if (pw->eqNodeId == id) {
            pw->eqNodeId = 0;
            pw->destroyNode(&pw->node);
            if (pw->owner != nullptr) {
                pw->owner->m_ready = false;
            }
        }
        if (pw->eqOutputNodeId == id) {
            pw->eqOutputNodeId = 0;
            pw->destroyNode(&pw->outputNode);
        }
        if (pw->hardwareNodeId == id) {
            pw->destroyNode(&pw->hardwareNode);
            pw->hardwareNodeId = 0;
        }
        for (auto it = pw->sinkIds.begin(); it != pw->sinkIds.end(); ) {
            if (it.value() == id) {
                it = pw->sinkIds.erase(it);
            } else {
                ++it;
            }
        }
    }

    bool waitUntil(const std::function<bool()> &pred, int timeoutMs = kPwTimeoutMs)
    {
        QElapsedTimer timer;
        timer.start();
        while (!pred()) {
            if (timer.elapsed() > timeoutMs) {
                return false;
            }
            pw_thread_loop_timed_wait(loop, 1);
        }
        return true;
    }

    bool sync()
    {
        if (core == nullptr) {
            return false;
        }
        syncDone = false;
        syncSeq = pw_core_sync(core, PW_ID_CORE, 0);
        return waitUntil([this] { return syncDone; });
    }

    bool syncVolume()
    {
        if (volumeCore == nullptr) {
            return false;
        }
        volumeSyncDone = false;
        volumeSyncSeq = pw_core_sync(volumeCore, PW_ID_CORE, 0);
        return waitUntil([this] { return volumeSyncDone; });
    }

    QString chooseHardwareSink() const
    {
        if (!defaultSinkName.isEmpty() && sinkNames.contains(defaultSinkName)) {
            return defaultSinkName;
        }
        if (!sinkNames.isEmpty()) {
            return sinkNames.first();
        }
        return {};
    }

    bool setDefaultSink(const QString &name)
    {
        if (metadata == nullptr || name.isEmpty()) {
            return false;
        }
        const QByteArray json = QString("{\"name\":\"%1\"}").arg(name).toUtf8();
        const int audio = pw_metadata_set_property(
            metadata, PW_ID_CORE, kDefaultSinkKey, "Spa:String:JSON", json.constData());
        const int configured = pw_metadata_set_property(
            metadata, PW_ID_CORE, kConfiguredSinkKey, "Spa:String:JSON", json.constData());
        return audio >= 0 && configured >= 0;
    }

    void restoreDefault()
    {
        if (!claimedDefault || hardwareSink.isEmpty()) {
            return;
        }
        setDefaultSink(hardwareSink);
        sync();
    }

    QString moduleArgs() const
    {
        QString nodes;
        QString links;
        nodes += "{ type = builtin name = pre label = linear control = { \"Mult\" = 1.0 \"Add\" = 0.0 } }\n";
        QString previous = "pre";
        for (int i = 0; i < EqBands::Count; ++i) {
            const QString name = QString("b%1").arg(i);
            nodes += QString("{ type = builtin name = %1 label = bq_peaking control = { \"Freq\" = %2 \"Q\" = %3 \"Gain\" = 0.0 } }\n")
                         .arg(name)
                         .arg(EqBands::FrequenciesHz[i], 0, 'f', 1)
                         .arg(EqBands::Q, 0, 'f', 1);
            links += QString("{ output = \"%1:Out\" input = \"%2:In\" }\n").arg(previous, name);
            previous = name;
        }

        QString targetLine;
        if (!hardwareSink.isEmpty()) {
            targetLine = QString("target.object = \"%1\"").arg(hardwareSink);
        }

        return QString(
                   "node.description = \"Linamp Equalizer\"\n"
                   "media.name = \"Linamp Equalizer\"\n"
                   "filter.graph = {\n"
                   "    nodes = [\n"
                   "%1"
                   "    ]\n"
                   "    links = [\n"
                   "%2"
                   "    ]\n"
                   "}\n"
                   "audio.channels = 2\n"
                   "audio.position = [ FL FR ]\n"
                   "capture.props = {\n"
                   "    node.name = \"linamp-eq\"\n"
                   "    node.description = \"Linamp Equalizer\"\n"
                   "    media.class = Audio/Sink\n"
                   "    state.restore-props = \"false\"\n"
                   "}\n"
                   "playback.props = {\n"
                   "    node.name = \"linamp-eq-output\"\n"
                   "    node.passive = true\n"
                   "    state.restore-props = \"false\"\n"
                   "    %3\n"
                   "}\n")
            .arg(nodes, links, targetLine);
    }

    bool setParams(const QHash<QString, double> &params)
    {
        if (node == nullptr || params.isEmpty()) {
            return false;
        }

        uint8_t buffer[2048];
        spa_pod_builder builder;
        spa_pod_builder_init(&builder, buffer, sizeof(buffer));

        spa_pod_frame objectFrame;
        spa_pod_builder_push_object(&builder, &objectFrame, SPA_TYPE_OBJECT_Props, SPA_PARAM_Props);

        spa_pod_frame paramsFrame;
        spa_pod_builder_prop(&builder, SPA_PROP_params, 0);
        spa_pod_builder_push_struct(&builder, &paramsFrame);
        for (auto it = params.cbegin(); it != params.cend(); ++it) {
            const QByteArray key = it.key().toUtf8();
            spa_pod_builder_string(&builder, key.constData());
            spa_pod_builder_float(&builder, float(it.value()));
        }
        spa_pod_builder_pop(&builder, &paramsFrame);

        auto *pod = static_cast<spa_pod *>(spa_pod_builder_pop(&builder, &objectFrame));
        if (pod == nullptr) {
            qWarning() << "PipeWire EQ failed to build Props pod";
            return false;
        }
        const int rc = pw_node_set_param(node, SPA_PARAM_Props, 0, pod);
        if (rc < 0) {
            qWarning() << "PipeWire EQ set-param failed:" << strerror(-rc);
            return false;
        }
        return true;
    }

    // Raise a node that WirePlumber left at its new-output default. Does not
    // touch mute, so a mute set elsewhere stays in place.
    bool setUnityVolume(pw_node *target)
    {
        if (target == nullptr) {
            return false;
        }

        const float volumes[2] = { 1.0f, 1.0f };
        uint8_t buffer[512];
        spa_pod_builder builder;
        spa_pod_builder_init(&builder, buffer, sizeof(buffer));

        spa_pod_frame objectFrame;
        spa_pod_builder_push_object(&builder, &objectFrame, SPA_TYPE_OBJECT_Props, SPA_PARAM_Props);
        spa_pod_builder_prop(&builder, SPA_PROP_volume, 0);
        spa_pod_builder_float(&builder, 1.0f);
        spa_pod_builder_prop(&builder, SPA_PROP_channelVolumes, 0);
        spa_pod_builder_array(&builder, sizeof(float), SPA_TYPE_Float, 2, volumes);

        auto *pod = static_cast<spa_pod *>(spa_pod_builder_pop(&builder, &objectFrame));
        if (pod == nullptr) {
            qWarning() << "PipeWire EQ failed to build the volume pod";
            return false;
        }
        const int rc = pw_node_set_param(target, SPA_PARAM_Props, 0, pod);
        if (rc < 0) {
            qWarning() << "PipeWire EQ failed to set unity volume:" << strerror(-rc);
            return false;
        }
        return true;
    }

    void destroyNode(pw_node **target)
    {
        if (target == nullptr || *target == nullptr) {
            return;
        }
        pw_proxy_destroy(reinterpret_cast<pw_proxy *>(*target));
        *target = nullptr;
    }

    void bindOutputNode()
    {
        if (outputNode != nullptr || eqOutputNodeId == 0 || registry == nullptr) {
            return;
        }
        outputNode = static_cast<pw_node *>(pw_registry_bind(
            registry, eqOutputNodeId, PW_TYPE_INTERFACE_Node, PW_VERSION_NODE, 0));
        if (outputNode != nullptr) {
            sync();
        }
    }

    void correctInitialVolumes()
    {
        bindOutputNode();
        qDebug() << "PipeWire EQ restoring unity sink volume";
        setUnityVolume(node);
        setUnityVolume(outputNode);
    }

    void bindHardware()
    {
        if (hardwareNode != nullptr || context == nullptr) {
            return;
        }
        const uint32_t id = sinkIds.value(hardwareSink, 0);
        if (id == 0) {
            return;
        }
        if (volumeCore == nullptr) {
            volumeCore = pw_context_connect(context, nullptr, 0);
            if (volumeCore == nullptr) {
                qWarning() << "PipeWire volume client failed to connect:" << strerror(errno);
                return;
            }
            static const pw_core_events events = {
                .version = PW_VERSION_CORE_EVENTS,
                .done = &Pw::onVolumeCoreDone,
                .error = &Pw::onCoreError,
            };
            pw_core_add_listener(volumeCore, &volumeCoreListener, &events, this);
            volumeCoreListenerAdded = true;
            volumeRegistry = pw_core_get_registry(volumeCore, PW_VERSION_REGISTRY, 0);
            if (volumeRegistry == nullptr || !syncVolume()) {
                qWarning() << "PipeWire volume client failed to sync";
                return;
            }
        }
        hardwareNode = static_cast<pw_node *>(pw_registry_bind(
            volumeRegistry, id, PW_TYPE_INTERFACE_Node, PW_VERSION_NODE, 0));
        if (hardwareNode == nullptr) {
            qWarning() << "PipeWire volume failed to bind" << hardwareSink;
            return;
        }
        hardwareNodeId = id;
        syncVolume();
        // Do not subscribe to Props. Playback emits volume updates, and a
        // listener that writes back spins this loop and wedges PipeWire.
        qDebug() << "PipeWire volume controlling hardware sink" << hardwareSink;
    }

    bool setHardwareChannelVolumes(float left, float right)
    {
        bindHardware();
        if (hardwareNode == nullptr) {
            return false;
        }
        // Applied gain is volume * channelVolumes. Slider goes in volume.
        const float peak = left >= right ? left : right;
        float channels[2];
        if (peak <= 1.0e-6f) {
            channels[0] = 1.0f;
            channels[1] = 1.0f;
        } else {
            channels[0] = left / peak;
            channels[1] = right / peak;
        }
        uint8_t buffer[512];
        spa_pod_builder builder;
        spa_pod_builder_init(&builder, buffer, sizeof(buffer));
        spa_pod_frame objectFrame;
        spa_pod_builder_push_object(&builder, &objectFrame, SPA_TYPE_OBJECT_Props, SPA_PARAM_Props);
        spa_pod_builder_prop(&builder, SPA_PROP_volume, 0);
        spa_pod_builder_float(&builder, peak);
        spa_pod_builder_prop(&builder, SPA_PROP_channelVolumes, 0);
        spa_pod_builder_array(&builder, sizeof(float), SPA_TYPE_Float, 2, channels);
        auto *pod = static_cast<spa_pod *>(spa_pod_builder_pop(&builder, &objectFrame));
        if (pod == nullptr) {
            return false;
        }
        const int rc = pw_node_set_param(hardwareNode, SPA_PARAM_Props, 0, pod);
        if (rc < 0) {
            qWarning() << "PipeWire volume set-param failed:" << strerror(-rc);
            return false;
        }
        return true;
    }

    // Tear down while the thread loop is still running so the daemon
    // actually receives the unexport. Stopping the loop first leaves
    // linamp-eq as a zombie default sink and wedges PipeWire.
    void shutdown()
    {
        restoreDefault();

        destroyNode(&hardwareNode);
        hardwareNodeId = 0;
        if (volumeCoreListenerAdded) {
            spa_hook_remove(&volumeCoreListener);
            volumeCoreListenerAdded = false;
        }
        if (volumeRegistry != nullptr) {
            pw_proxy_destroy(reinterpret_cast<pw_proxy *>(volumeRegistry));
            volumeRegistry = nullptr;
        }
        if (volumeCore != nullptr) {
            pw_core_disconnect(volumeCore);
            volumeCore = nullptr;
        }

        destroyNode(&node);
        destroyNode(&outputNode);

        if (metadataListenerAdded) {
            spa_hook_remove(&metadataListener);
            metadataListenerAdded = false;
        }
        if (metadata != nullptr) {
            pw_proxy_destroy(reinterpret_cast<pw_proxy *>(metadata));
            metadata = nullptr;
        }

        if (module != nullptr) {
            pw_impl_module_schedule_destroy(module);
            module = nullptr;
            waitUntil([this] { return eqNodeId == 0 && eqOutputNodeId == 0; });
        }

        if (registryListenerAdded) {
            spa_hook_remove(&registryListener);
            registryListenerAdded = false;
        }
        if (registry != nullptr) {
            pw_proxy_destroy(reinterpret_cast<pw_proxy *>(registry));
            registry = nullptr;
        }
        if (coreListenerAdded) {
            spa_hook_remove(&coreListener);
            coreListenerAdded = false;
        }
        if (core != nullptr) {
            pw_core_disconnect(core);
            core = nullptr;
        }
    }
};

SystemEqualizer::SystemEqualizer(QObject *parent)
    : QObject(parent)
{
    loadSettings();

    m_applyTimer = new QTimer(this);
    m_applyTimer->setInterval(30);
    m_applyTimer->setSingleShot(true);
    connect(m_applyTimer, &QTimer::timeout, this, &SystemEqualizer::applyPending);

    if (!startPipeWire() && m_enabled) {
        qWarning() << "Equalizer settings say on, but the PipeWire filter is not available";
    }
}

SystemEqualizer::~SystemEqualizer()
{
    m_ready = false;
    if (m_applyTimer != nullptr) {
        m_applyTimer->stop();
    }
    if (m_pw == nullptr) {
        return;
    }

    Pw *pw = m_pw;
    if (pw->loop != nullptr && pw->loopStarted) {
        pw_thread_loop_lock(pw->loop);
        pw->shutdown();
        pw_thread_loop_unlock(pw->loop);
        pw_thread_loop_stop(pw->loop);
    }

    if (pw->context != nullptr) {
        pw_context_destroy(pw->context);
        pw->context = nullptr;
    }
    if (pw->loop != nullptr) {
        pw_thread_loop_destroy(pw->loop);
        pw->loop = nullptr;
    }
    if (pw->pwInit) {
        pw_deinit();
        pw->pwInit = false;
    }
    delete pw;
    m_pw = nullptr;
}

bool SystemEqualizer::isEnabled() const
{
    return m_ready && m_enabled;
}

QString SystemEqualizer::hardwareSink() const
{
    return m_hardwareSink;
}

double SystemEqualizer::preampDb() const
{
    return m_preampDb;
}

double SystemEqualizer::bandDb(int index) const
{
    if (index < 0 || index >= EqBands::Count) {
        return 0.0;
    }
    return m_bandDb[index];
}

void SystemEqualizer::setEnabled(bool enabled)
{
    if (m_enabled == enabled && !(enabled && !m_ready)) {
        return;
    }
    m_enabled = enabled;
    saveEnabled();
    m_pending.clear();
    m_applyTimer->stop();
    if (m_ready) {
        applyAll();
    }
    emit enabledChanged(isEnabled());
}

void SystemEqualizer::setPreampDb(double db)
{
    db = EqBands::clampDb(db);
    m_preampDb = db;
    savePreamp();
    if (!m_enabled || !m_ready) {
        return;
    }
    queueParam("pre:Mult", EqBands::dbToLinear(db));
}

void SystemEqualizer::setBandDb(int index, double db)
{
    if (index < 0 || index >= EqBands::Count) {
        return;
    }
    db = EqBands::clampDb(db);
    m_bandDb[index] = db;
    saveBand(index);
    if (!m_enabled || !m_ready) {
        return;
    }
    queueParam(bandGainKey(index), db);
}

void SystemEqualizer::flush()
{
    m_applyTimer->stop();
    m_pending.clear();
    m_settings.sync();
    if (m_ready) {
        applyAll();
    }
}

void SystemEqualizer::setHardwareVolume(int volume, int balance)
{
    if (m_pw == nullptr || m_pw->loop == nullptr) {
        return;
    }
    float left = 1.0f;
    float right = 1.0f;
    volumeToChannels(volume, balance, &left, &right);
    pw_thread_loop_lock(m_pw->loop);
    m_pw->setHardwareChannelVolumes(left, right);
    pw_thread_loop_unlock(m_pw->loop);
}

void SystemEqualizer::applyPending()
{
    if (!m_ready || m_pw == nullptr || m_pw->loop == nullptr || m_pending.isEmpty()) {
        return;
    }
    const QHash<QString, double> pending = m_pending;
    m_pending.clear();
    pw_thread_loop_lock(m_pw->loop);
    m_pw->setParams(pending);
    pw_thread_loop_unlock(m_pw->loop);
}

void SystemEqualizer::loadSettings()
{
    m_settings.beginGroup("equalizer");
    m_enabled = m_settings.value("enabled", false).toBool();
    m_preampDb = EqBands::clampDb(m_settings.value("preamp", 0.0).toDouble());
    for (int i = 0; i < EqBands::Count; ++i) {
        m_bandDb[i] = EqBands::clampDb(m_settings.value(QString("band%1").arg(i), 0.0).toDouble());
    }
    m_settings.endGroup();
}

void SystemEqualizer::saveEnabled()
{
    m_settings.beginGroup("equalizer");
    m_settings.setValue("enabled", m_enabled);
    m_settings.endGroup();
    m_settings.sync();
}

void SystemEqualizer::savePreamp()
{
    m_settings.beginGroup("equalizer");
    m_settings.setValue("preamp", m_preampDb);
    m_settings.endGroup();
}

void SystemEqualizer::saveBand(int index)
{
    m_settings.beginGroup("equalizer");
    m_settings.setValue(QString("band%1").arg(index), m_bandDb[index]);
    m_settings.endGroup();
}

void SystemEqualizer::queueParam(const QString &key, double value)
{
    m_pending.insert(key, value);
    if (!m_applyTimer->isActive()) {
        m_applyTimer->start();
    }
}

void SystemEqualizer::applyAll()
{
    if (!m_ready || m_pw == nullptr || m_pw->loop == nullptr) {
        return;
    }

    QHash<QString, double> params;
    if (m_enabled) {
        params.insert("pre:Mult", EqBands::dbToLinear(m_preampDb));
        for (int i = 0; i < EqBands::Count; ++i) {
            params.insert(bandGainKey(i), m_bandDb[i]);
        }
    } else {
        params.insert("pre:Mult", 1.0);
        for (int i = 0; i < EqBands::Count; ++i) {
            params.insert(bandGainKey(i), 0.0);
        }
    }

    pw_thread_loop_lock(m_pw->loop);
    m_pw->setParams(params);
    pw_thread_loop_unlock(m_pw->loop);
}

void SystemEqualizer::correctInitialSinkVolume()
{
    if (!m_ready || m_pw == nullptr || m_pw->loop == nullptr) {
        return;
    }
    pw_thread_loop_lock(m_pw->loop);
    m_pw->correctInitialVolumes();
    pw_thread_loop_unlock(m_pw->loop);
}

bool SystemEqualizer::startPipeWire()
{
    m_pw = new Pw(this);
    Pw *pw = m_pw;

    pw_init(nullptr, nullptr);
    pw->pwInit = true;

    pw->loop = pw_thread_loop_new("linamp-eq", nullptr);
    if (pw->loop == nullptr) {
        qWarning() << "PipeWire EQ failed to create a thread loop";
        return false;
    }

    pw->context = pw_context_new(pw_thread_loop_get_loop(pw->loop), nullptr, 0);
    if (pw->context == nullptr) {
        qWarning() << "PipeWire EQ failed to create a context:" << strerror(errno);
        return false;
    }

    if (pw_thread_loop_start(pw->loop) < 0) {
        qWarning() << "PipeWire EQ failed to start the thread loop";
        return false;
    }
    pw->loopStarted = true;

    pw_thread_loop_lock(pw->loop);

    pw->core = pw_context_connect(pw->context, nullptr, 0);
    if (pw->core == nullptr) {
        qWarning() << "PipeWire EQ failed to connect:" << strerror(errno);
        pw_thread_loop_unlock(pw->loop);
        return false;
    }

    static const pw_core_events coreEvents = {
        .version = PW_VERSION_CORE_EVENTS,
        .done = &Pw::onCoreDone,
        .error = &Pw::onCoreError,
    };
    pw_core_add_listener(pw->core, &pw->coreListener, &coreEvents, pw);
    pw->coreListenerAdded = true;

    pw->registry = pw_core_get_registry(pw->core, PW_VERSION_REGISTRY, 0);
    if (pw->registry == nullptr) {
        qWarning() << "PipeWire EQ failed to get the registry";
        pw_thread_loop_unlock(pw->loop);
        return false;
    }

    static const pw_registry_events registryEvents = {
        .version = PW_VERSION_REGISTRY_EVENTS,
        .global = &Pw::onGlobal,
        .global_remove = &Pw::onGlobalRemove,
    };
    pw_registry_add_listener(pw->registry, &pw->registryListener, &registryEvents, pw);
    pw->registryListenerAdded = true;

    if (!pw->sync()) {
        qWarning() << "PipeWire EQ timed out reading the graph";
        pw_thread_loop_unlock(pw->loop);
        return false;
    }

    pw->hardwareSinkChosen = true;
    pw->hardwareSink = pw->chooseHardwareSink();
    if (pw->hardwareSink.isEmpty()) {
        qWarning() << "PipeWire EQ found no hardware sink to target";
        pw_thread_loop_unlock(pw->loop);
        return false;
    }

    if (pw->eqNodeId != 0 || pw->eqOutputNodeId != 0) {
        qDebug() << "PipeWire EQ waiting for leftover equalizer nodes to leave";
        pw->waitUntil([pw] { return pw->eqNodeId == 0 && pw->eqOutputNodeId == 0; });
    }

    if (pw->eqNodeId == 0) {
        const QByteArray args = pw->moduleArgs().toUtf8();
        qDebug() << "PipeWire EQ loading filter-chain, playback target" << pw->hardwareSink;
        pw->module = pw_context_load_module(pw->context, "libpipewire-module-filter-chain", args.constData(), nullptr);
        if (pw->module == nullptr) {
            qWarning() << "PipeWire EQ failed to load filter-chain:" << strerror(errno);
            qWarning().noquote() << pw->moduleArgs();
            pw_thread_loop_unlock(pw->loop);
            return false;
        }
        qDebug() << "PipeWire EQ filter-chain module loaded";
        if (!pw->waitUntil([pw] { return pw->eqNodeId != 0 && pw->eqOutputNodeId != 0; })) {
            qWarning() << "PipeWire EQ timed out waiting for equalizer nodes, sink" << pw->eqNodeId
                       << "output" << pw->eqOutputNodeId;
            if (pw->eqNodeId == 0) {
                pw_thread_loop_unlock(pw->loop);
                return false;
            }
        }
    } else {
        qDebug() << "PipeWire EQ reusing existing linamp-eq sink";
    }

    pw->node = static_cast<pw_node *>(pw_registry_bind(
        pw->registry, pw->eqNodeId, PW_TYPE_INTERFACE_Node, PW_VERSION_NODE, 0));
    if (pw->node == nullptr) {
        qWarning() << "PipeWire EQ failed to bind the sink node";
        pw_thread_loop_unlock(pw->loop);
        return false;
    }
    if (!pw->sync()) {
        qWarning() << "PipeWire EQ timed out binding the sink";
        pw_thread_loop_unlock(pw->loop);
        return false;
    }

    if (!pw->setDefaultSink(QString::fromLatin1(kEqNodeName))) {
        qWarning() << "PipeWire EQ failed to set the default sink";
        pw_thread_loop_unlock(pw->loop);
        return false;
    }
    pw->claimedDefault = true;
    pw->sync();
    pw->correctInitialVolumes();
    pw->bindHardware();

    pw_thread_loop_unlock(pw->loop);

    m_ready = true;
    m_hardwareSink = pw->hardwareSink;
    applyAll();
    // WirePlumber can lower a new node after it appears. One-shot writes
    // only; do not subscribe, or playback volume events loop forever.
    QTimer::singleShot(250, this, [this] { correctInitialSinkVolume(); });
    QTimer::singleShot(1000, this, [this] { correctInitialSinkVolume(); });
    QTimer::singleShot(2500, this, [this] { correctInitialSinkVolume(); });
    qDebug() << "PipeWire EQ ready, hardware sink" << pw->hardwareSink << "enabled" << m_enabled;
    emit hardwareSinkChanged(m_hardwareSink);
    return true;
}

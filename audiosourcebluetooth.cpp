#include "audiosourcebluetooth.h"
#include "spectrumwidget.h"
#include "util.h"

#include <pulse/error.h>
#include <pulse/pulseaudio.h>
#include <pulse/simple.h>
#include <QtConcurrent>

#define PA_SAMPLE_RATE DEFAULT_SAMPLE_RATE
#define PA_CHANNELS 2

static QMutex *sampleMutex;
static QByteArray *sample;
static QDataStream *sampleStream;
static bool captureRunning = false;
static pa_mainloop *paLoop;

static void pa_stream_notify_cb(pa_stream *stream, void* /*userdata*/)
{
    const pa_stream_state state = pa_stream_get_state(stream);
    switch (state) {
    case PA_STREAM_FAILED:
        qDebug() << "Stream failed";
        break;
    case PA_STREAM_READY:
        qDebug() << "Stream ready";
        break;
    default:
        qDebug() << "Stream state: " << state;
    }
}

static void pa_stream_read_cb(pa_stream *stream, const size_t /*nbytes*/, void* /*userdata*/)
{
    if(sampleMutex == nullptr || sample == nullptr || sampleStream == nullptr) {
        return;
    }
    QMutexLocker l(sampleMutex);

    // Careful when to pa_stream_peek() and pa_stream_drop()!
    // c.f. https://www.freedesktop.org/software/pulseaudio/doxygen/stream_8h.html#ac2838c449cde56e169224d7fe3d00824
    int16_t *data = nullptr;
    size_t actualbytes = 0;
    if (pa_stream_peek(stream, (const void**)&data, &actualbytes) != 0) {
        qDebug() << "Failed to peek at stream data";
        return;
    }

    if (data == nullptr && actualbytes == 0) {
        // No data in the buffer, ignore.
        return;
    } else if (data == nullptr && actualbytes > 0) {
        // Hole in the buffer. We must drop it.
        if (pa_stream_drop(stream) != 0) {
            qDebug() << "Failed to drop a hole! (Sounds weird, doesn't it?)";
            return;
        }
    }

    // process data
    //qDebug() << ">> " << actualbytes << " bytes";
    if(actualbytes >= DFT_SIZE * 4) {
        // If the sample is equal or bigger than the target
        // Replace the current buffer
        sample->clear();
        delete sampleStream;
        sampleStream = new QDataStream(sample, QIODevice::WriteOnly);
    }
    sampleStream->writeRawData((const char *)data, actualbytes);

    if (pa_stream_drop(stream) != 0) {
        qDebug() << "Failed to drop data after peeking.";
    }
}

static void pa_server_info_cb(pa_context *ctx, const pa_server_info *info, void* /*userdata*/)
{
    qDebug() << "Default sink: " << info->default_sink_name;

    pa_sample_spec spec;
    spec.format = PA_SAMPLE_S16LE;
    spec.rate = PA_SAMPLE_RATE;
    spec.channels = PA_CHANNELS;
    // Use pa_stream_new_with_proplist instead?
    pa_stream *stream = pa_stream_new(ctx, "output monitor", &spec, nullptr);

    pa_stream_set_state_callback(stream, &pa_stream_notify_cb, nullptr /*userdata*/);
    pa_stream_set_read_callback(stream, &pa_stream_read_cb, nullptr /*userdata*/);

    std::string monitor_name(info->default_sink_name);
    monitor_name += ".monitor";
    pa_buffer_attr bufferAttr;
    // Limit max data so spectrumwidget paints frequently
    bufferAttr.maxlength = MAX_AUDIO_STREAM_SAMPLE_SIZE;
    bufferAttr.fragsize = MAX_AUDIO_STREAM_SAMPLE_SIZE;
    if (pa_stream_connect_record(stream, monitor_name.c_str(), &bufferAttr, PA_STREAM_ADJUST_LATENCY) != 0) {
        qDebug() << "connection fail";
        return;
    }

    qDebug() << "Connected to " << monitor_name.c_str();
}

static void pa_context_notify_cb(pa_context *ctx, void* /*userdata*/)
{
    const pa_context_state state = pa_context_get_state(ctx);
    switch (state) {
    case PA_CONTEXT_READY:
        qDebug() << "Context ready";
        pa_context_get_server_info(ctx, &pa_server_info_cb, nullptr /*userdata*/);
        break;
    case PA_CONTEXT_FAILED:
        qDebug() << "Context failed";
        break;
    default:
        qDebug() << "Context state: " << state;
    }
}

static bool isValidSample(QByteArray *sample)
{
    // Greedy: suposing that 100 is enough
    for(int i = 0; i < 100; i++) {
        if(sample->at(i) != 0) {
            return true;
        }
    }
    return false;
}

int startPACapture()
{
    captureRunning = true;

    paLoop = pa_mainloop_new();
    pa_mainloop_api *api = pa_mainloop_get_api(paLoop);
    pa_context *ctx = pa_context_new(api, "padump");
    pa_context_set_state_callback(ctx, &pa_context_notify_cb, nullptr /*userdata*/);
    if (pa_context_connect(ctx, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0) {
        qDebug() << "PA connection failed.";
        return -1;
    }

    pa_mainloop_run(paLoop, nullptr);

    pa_context_disconnect(ctx);
    pa_mainloop_free(paLoop);
    return 0;
}

void stopPACapture()
{
    if(!captureRunning) {
        return;
    }

    pa_mainloop_quit(paLoop, 0);
    if(sample != nullptr) {
        sample->clear();
    }
    if(sampleStream) {
        delete sampleStream;
        sampleStream = new QDataStream(sample, QIODevice::WriteOnly);
    }
    captureRunning = false;
}

AudioSourceBluetooth::AudioSourceBluetooth(QObject *parent)
    : AudioSource{parent}
{
    sampleMutex = new QMutex();
    sample = new QByteArray();
    sampleStream = new QDataStream(sample, QIODevice::WriteOnly);
    dataEmitTimer = new QTimer(this);
    dataEmitTimer->setInterval(33); // around 30 fps
    connect(dataEmitTimer, &QTimer::timeout, this, &AudioSourceBluetooth::emitData);

    // Init dbus
    auto dbusConn = QDBusConnection::systemBus();
    if(!dbusConn.isConnected()) {
        qDebug() << "Cannot connect to DBuss";
        return;
    } else {
        qDebug() << "CONNECTED!";
    }

    this->dbusIface = new QDBusInterface(SERVICE_NAME, OBJ_PATH, OBJ_INTERFACE, dbusConn, this);
    if(!this->dbusIface->isValid()) {
        qDebug() << "DBus interface is invalid";
        return;
    }

    bool success = dbusConn.connect(SERVICE_NAME, OBJ_PATH, "org.freedesktop.DBus.Properties", "PropertiesChanged", this, SLOT(btStatusChanged(QString, QVariantMap, QStringList)));

    if(success) {
        qDebug() << "SUCCESS!";
    } else {
        qDebug() << "MEH";
    }
}

AudioSourceBluetooth::~AudioSourceBluetooth()
{
    stopPACapture();
}

void AudioSourceBluetooth::btStatusChanged(QString name, QVariantMap map, QStringList list)
{
    qDebug() << QString("properties of interface %1 changed").arg(name);
    for (QVariantMap::const_iterator it = map.cbegin(), end = map.cend(); it != end; ++it) {
        qDebug() << "property: " << it.key() << " value: " << it.value();

        QString prop = it.key();

        if(prop == "Status") {
            QString status = it.value().toString();
            if(status == "playing") {
                emit this->playbackStateChanged(MediaPlayer::PlaybackState::PlayingState);
            }
            if(status == "stopped") {
                emit this->playbackStateChanged(MediaPlayer::PlaybackState::StoppedState);
            }
            if(status == "paused") {
                emit this->playbackStateChanged(MediaPlayer::PlaybackState::PausedState);
            }
            if(status == "error") {
                emit this->messageSet("Bluetooth Error", 5000);
            }
        }

        if(prop == "Track") {
            QMediaMetaData metadata;
            QVariantMap trackMap = qdbus_cast<QVariantMap>(it.value());
            for( QString trackKey : trackMap.keys()){
                qDebug() << "    " << trackKey << ":" << trackMap.value(trackKey);
                if(trackKey == "Title") {
                    metadata.insert(QMediaMetaData::Title, trackMap.value(trackKey));
                }
                if(trackKey == "Artist") {
                    metadata.insert(QMediaMetaData::AlbumArtist, trackMap.value(trackKey));
                }
                if(trackKey == "Album") {
                    metadata.insert(QMediaMetaData::AlbumTitle, trackMap.value(trackKey));
                }
                if(trackKey == "Genre") {
                    metadata.insert(QMediaMetaData::Genre, trackMap.value(trackKey));
                }
                if(trackKey == "TrackNumber") {
                    metadata.insert(QMediaMetaData::TrackNumber, trackMap.value(trackKey));
                }
                if(trackKey == "Duration") {
                    quint32 duration = trackMap.value(trackKey).toUInt();
                    metadata.insert(QMediaMetaData::Duration, duration);
                    emit this->durationChanged(duration);
                }
            }
            emit this->metadataChanged(metadata);
        }

        if(prop == "Repeat") {
            QString repeatSetting = it.value().toString();
            if(repeatSetting == "off") {
                emit this->repeatEnabledChanged(false);
                this->isRepeatEnabled = false;
            } else {
                emit this->repeatEnabledChanged(true);
                this->isRepeatEnabled = true;
            }
        }

        if(prop == "Shuffle") {
            QString shuffleSetting = it.value().toString();
            if(shuffleSetting == "off") {
                emit this->shuffleEnabledChanged(false);
                this->isShuffleEnabled = false;
            } else {
                emit this->shuffleEnabledChanged(true);
                this->isShuffleEnabled = true;
            }
        }

    }
    for (const auto& element : list) {
        qDebug() << "list element: " << element;
    }
}


void AudioSourceBluetooth::activate()
{
    QMediaMetaData metadata = QMediaMetaData{};
    metadata.insert(QMediaMetaData::Title, "Bluetooth");

    emit playbackStateChanged(MediaPlayer::StoppedState);
    emit positionChanged(0);
    emit metadataChanged(metadata);
    emit durationChanged(0);
    emit eqEnabledChanged(false);
    emit plEnabledChanged(false);
    emit shuffleEnabledChanged(false);
    emit repeatEnabledChanged(false);

    this->isShuffleEnabled = false;
    this->isRepeatEnabled = false;
}

void AudioSourceBluetooth::deactivate()
{
    stopPACapture();
    dataEmitTimer->stop();
    emit playbackStateChanged(MediaPlayer::StoppedState);
}

void AudioSourceBluetooth::handlePl()
{
    emit plEnabledChanged(false);
}

void AudioSourceBluetooth::handlePrevious()
{
    if(this->dbusIface->isValid()) {
        this->dbusIface->call("Previous");
    }
}

void AudioSourceBluetooth::handlePlay()
{
    QtConcurrent::run(startPACapture);
    dataEmitTimer->start();
    if(this->dbusIface->isValid()) {
        this->dbusIface->call("Play");
    }
    emit playbackStateChanged(MediaPlayer::PlayingState);
}

void AudioSourceBluetooth::handlePause()
{
    stopPACapture();
    dataEmitTimer->stop();
    if(this->dbusIface->isValid()) {
        this->dbusIface->call("Pause");
    }
    emit playbackStateChanged(MediaPlayer::PausedState);
}

void AudioSourceBluetooth::handleStop()
{
    stopPACapture();
    dataEmitTimer->stop();
    if(this->dbusIface->isValid()) {
        this->dbusIface->call("Stop");
    }
    emit playbackStateChanged(MediaPlayer::StoppedState);
}

void AudioSourceBluetooth::handleNext()
{
    if(this->dbusIface->isValid()) {
        this->dbusIface->call("Next");
    }
}

void AudioSourceBluetooth::handleOpen()
{

}

void AudioSourceBluetooth::handleShuffle()
{
    this->isShuffleEnabled = !this->isShuffleEnabled;
    emit shuffleEnabledChanged(this->isShuffleEnabled);
    if(this->dbusIface->isValid()) {
        this->dbusIface->setProperty("Shuffle", this->isShuffleEnabled ? "alltracks" : "off");
    }
}

void AudioSourceBluetooth::handleRepeat()
{
    this->isRepeatEnabled = !this->isRepeatEnabled;
    emit repeatEnabledChanged(this->isRepeatEnabled);
    if(this->dbusIface->isValid()) {
        this->dbusIface->setProperty("Repeat", this->isRepeatEnabled ? "alltracks" : "off");
    }
}

void AudioSourceBluetooth::handleSeek(int mseconds)
{

}

void AudioSourceBluetooth::emitData()
{
    QMutexLocker l(sampleMutex);

    if(sample->length() < DFT_SIZE * 4) {
        return;
    }

    if(!isValidSample(sample)) {
        return;
    }

    //dbg
    /*QString dbgsample = "";
    for(int i = 0; i < 100; i++) {
        dbgsample += QString(sample->at(i)) + ",";
    }
    qDebug() << ">>>>>" << dbgsample;
    */
    // end dbg

    QAudioFormat format;
    format.setSampleFormat(QAudioFormat::Int16);
    format.setSampleRate(PA_SAMPLE_RATE);
    format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    format.setChannelCount(PA_CHANNELS);
    emit dataEmitted(*sample, format);
    sample->clear();
    delete sampleStream;
    sampleStream = new QDataStream(sample, QIODevice::WriteOnly);
}

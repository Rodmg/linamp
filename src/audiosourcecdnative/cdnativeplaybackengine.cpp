#include "cdnativeplaybackengine.h"

#include <cdio/paranoia/cdda.h>
#include <cdio/paranoia/paranoia.h>

#include <QMediaDevices>
#include <QRandomGenerator>
#include <QThread>
#include <QtConcurrent>

#include "cdpcmiodevice.h"
#include "cdpcmringbuffer.h"

namespace {
constexpr int CD_BYTES_PER_SECTOR = 2352;
constexpr int READER_CHUNK_SECTORS = 32;
constexpr int POSITION_TICK_MS = 100;
}

CDNativePlaybackEngine::CDNativePlaybackEngine(QObject *parent)
    : QObject(parent)
    , m_ringBuffer(new CDPcmRingBuffer(CD_BYTES_PER_SECTOR * 512))
    , m_pcmDevice(new CDPcmIODevice(m_ringBuffer, this))
{
    m_audioFormat.setSampleRate(44100);
    m_audioFormat.setChannelCount(2);
    m_audioFormat.setSampleFormat(QAudioFormat::Int16);

    m_positionTimer.setInterval(POSITION_TICK_MS);
    connect(&m_positionTimer, &QTimer::timeout, this, &CDNativePlaybackEngine::updatePositionTick);

    m_pcmDevice->open(QIODevice::ReadOnly);
}

CDNativePlaybackEngine::~CDNativePlaybackEngine()
{
    stopReaderLoop();

    if (m_audioSink != nullptr) {
        m_audioSink->stop();
        delete m_audioSink;
        m_audioSink = nullptr;
    }

    delete m_ringBuffer;
    m_ringBuffer = nullptr;
}

void CDNativePlaybackEngine::setTracks(const QList<CDNativeTrack> &tracks)
{
    m_tracks = tracks;
    m_positionMs = 0;
    m_currentLogicalIndex = 0;
    m_currentOrderIndex = 0;

    qint64 total = 0;
    for (const CDNativeTrack &track : m_tracks) {
        total += track.durationMs;
    }
    m_durationMs = total;

    rebuildPlayOrder();
    m_currentOrderIndex = findOrderIndexForTrack(m_currentLogicalIndex);
    emit durationChanged(m_durationMs);
    emit positionChanged(m_positionMs);
    emit activeTrackChanged(resolveTrackIndexForPosition(m_positionMs));
}

QList<CDNativeTrack> CDNativePlaybackEngine::tracks() const
{
    return m_tracks;
}

void CDNativePlaybackEngine::setShuffleEnabled(bool enabled)
{
    if (m_shuffleEnabled.load() == enabled) {
        return;
    }
    m_shuffleEnabled.store(enabled);
    rebuildPlayOrder();
}

void CDNativePlaybackEngine::setRepeatAllEnabled(bool enabled)
{
    m_repeatAllEnabled.store(enabled);
}

bool CDNativePlaybackEngine::shuffleEnabled() const
{
    return m_shuffleEnabled.load();
}

bool CDNativePlaybackEngine::repeatAllEnabled() const
{
    return m_repeatAllEnabled.load();
}

void CDNativePlaybackEngine::play()
{
    if (m_tracks.isEmpty()) {
        return;
    }

    ensureSink();
    m_positionElapsed.restart();
    m_positionTimer.start();

    startReaderLoop();

    if (m_audioSink != nullptr) {
        m_audioSink->start(m_pcmDevice);
    }
}

void CDNativePlaybackEngine::pause()
{
    if (m_audioSink != nullptr) {
        m_audioSink->suspend();
    }
    m_positionTimer.stop();
}

void CDNativePlaybackEngine::stop()
{
    m_positionTimer.stop();
    m_positionMs = 0;
    m_currentLogicalIndex = 0;

    if (m_audioSink != nullptr) {
        m_audioSink->stop();
    }

    stopReaderLoop();
    m_ringBuffer->clear();

    emit positionChanged(m_positionMs);
    emit activeTrackChanged(resolveTrackIndexForPosition(m_positionMs));
}

void CDNativePlaybackEngine::next()
{
    if (m_tracks.isEmpty()) {
        return;
    }

    const int currentIndex = resolveTrackIndexForPosition(m_positionMs);
    if (currentIndex < 0) {
        return;
    }

    int nextIndex = qMin(currentIndex + 1, m_tracks.size() - 1);
    if (m_shuffleEnabled.load() && !m_playOrder.isEmpty()) {
        m_currentOrderIndex = findOrderIndexForTrack(currentIndex);
        const int lastOrderIndex = m_playOrder.size() - 1;
        if (m_currentOrderIndex >= lastOrderIndex) {
            m_currentOrderIndex = m_repeatAllEnabled.load() ? 0 : lastOrderIndex;
        } else {
            ++m_currentOrderIndex;
        }
        nextIndex = m_playOrder.at(m_currentOrderIndex);
    }
    seek(trackStartMs(nextIndex));
}

void CDNativePlaybackEngine::previous()
{
    if (m_tracks.isEmpty()) {
        return;
    }

    const int currentIndex = resolveTrackIndexForPosition(m_positionMs);
    if (currentIndex < 0) {
        return;
    }

    int previousIndex = qMax(currentIndex - 1, 0);
    if (m_shuffleEnabled.load() && !m_playOrder.isEmpty()) {
        m_currentOrderIndex = findOrderIndexForTrack(currentIndex);
        if (m_currentOrderIndex <= 0) {
            m_currentOrderIndex = m_repeatAllEnabled.load() ? m_playOrder.size() - 1 : 0;
        } else {
            --m_currentOrderIndex;
        }
        previousIndex = m_playOrder.at(m_currentOrderIndex);
    }
    seek(trackStartMs(previousIndex));
}

void CDNativePlaybackEngine::seek(qint64 mseconds)
{
    if (m_tracks.isEmpty()) {
        return;
    }

    const qint64 clamped = qBound<qint64>(0, mseconds, m_durationMs);
    m_positionMs = clamped;
    m_currentLogicalIndex = qMax(0, resolveTrackIndexForPosition(m_positionMs));
    m_currentOrderIndex = findOrderIndexForTrack(m_currentLogicalIndex);

    // Jump/seek is intentionally non-gapless. We reset transport to the new absolute position.
    const bool wasRunning = m_positionTimer.isActive();
    stopReaderLoop();
    resetSinkStream();
    if (wasRunning) {
        startReaderLoop();
    }

    emit positionChanged(m_positionMs);
    emit activeTrackChanged(m_currentLogicalIndex);
}

qint64 CDNativePlaybackEngine::position() const
{
    return m_positionMs;
}

qint64 CDNativePlaybackEngine::duration() const
{
    return m_durationMs;
}

int CDNativePlaybackEngine::currentTrackIndex() const
{
    return resolveTrackIndexForPosition(m_positionMs);
}

void CDNativePlaybackEngine::rebuildPlayOrder()
{
    m_playOrder.clear();
    m_playOrder.reserve(m_tracks.size());

    for (int i = 0; i < m_tracks.size(); ++i) {
        m_playOrder.append(i);
    }

    if (!m_shuffleEnabled.load() || m_tracks.size() < 2) {
        m_currentOrderIndex = findOrderIndexForTrack(m_currentLogicalIndex);
        return;
    }

    for (int i = m_playOrder.size() - 1; i > 0; --i) {
        const int j = QRandomGenerator::global()->bounded(i + 1);
        m_playOrder.swapItemsAt(i, j);
    }

    m_currentOrderIndex = findOrderIndexForTrack(m_currentLogicalIndex);
}

int CDNativePlaybackEngine::findOrderIndexForTrack(int trackIndex) const
{
    for (int i = 0; i < m_playOrder.size(); ++i) {
        if (m_playOrder.at(i) == trackIndex) {
            return i;
        }
    }
    return 0;
}

int CDNativePlaybackEngine::resolveTrackIndexForPosition(qint64 positionMs) const
{
    if (m_tracks.isEmpty()) {
        return -1;
    }

    qint64 acc = 0;
    for (int i = 0; i < m_tracks.size(); ++i) {
        const qint64 end = acc + m_tracks.at(i).durationMs;
        if (positionMs < end || i == m_tracks.size() - 1) {
            return i;
        }
        acc = end;
    }

    return m_tracks.size() - 1;
}

qint64 CDNativePlaybackEngine::trackStartMs(int index) const
{
    if (index <= 0) {
        return 0;
    }

    qint64 acc = 0;
    for (int i = 0; i < index && i < m_tracks.size(); ++i) {
        acc += m_tracks.at(i).durationMs;
    }
    return acc;
}

void CDNativePlaybackEngine::updatePositionTick()
{
    if (!m_positionElapsed.isValid()) {
        m_positionElapsed.restart();
        return;
    }

    const qint64 elapsed = m_positionElapsed.restart();
    m_positionMs += elapsed;

    if (m_positionMs >= m_durationMs) {
        if (m_repeatAllEnabled.load() && m_durationMs > 0) {
            m_positionMs = 0;
            stopReaderLoop();
            resetSinkStream();
            startReaderLoop();
        } else {
            m_positionMs = m_durationMs;
            m_positionTimer.stop();
            if (m_audioSink != nullptr) {
                m_audioSink->stop();
            }
            stopReaderLoop();
        }
    }

    const int activeTrack = resolveTrackIndexForPosition(m_positionMs);
    if (activeTrack != m_currentLogicalIndex) {
        m_currentLogicalIndex = activeTrack;
        emit activeTrackChanged(m_currentLogicalIndex);
    }

    emit positionChanged(m_positionMs);
}

void CDNativePlaybackEngine::ensureSink()
{
    if (m_audioSink != nullptr) {
        return;
    }

    m_audioSink = new QAudioSink(QMediaDevices::defaultAudioOutput(), m_audioFormat, this);
    m_audioSink->setBufferSize(CD_BYTES_PER_SECTOR * 256);
}

void CDNativePlaybackEngine::resetSinkStream()
{
    m_ringBuffer->clear();

    if (m_audioSink == nullptr) {
        return;
    }

    const bool wasRunning = m_positionTimer.isActive();
    m_audioSink->stop();
    if (wasRunning) {
        m_audioSink->start(m_pcmDevice);
    }
}

void CDNativePlaybackEngine::startReaderLoop()
{
    if (m_readerRunning.load()) {
        return;
    }

    m_readerStopRequested.store(false);
    m_readerRunning.store(true);
    m_readerFuture = QtConcurrent::run([this]() {
        readerLoop();
    });
}

void CDNativePlaybackEngine::stopReaderLoop()
{
    if (!m_readerRunning.load()) {
        return;
    }

    m_readerStopRequested.store(true);
    m_readerFuture.waitForFinished();
    m_readerRunning.store(false);
}

void CDNativePlaybackEngine::readerLoop()
{
    const QByteArray deviceName = m_devicePath.toLocal8Bit();
    char *messages = nullptr;
    cdrom_drive_t *drive = cdio_cddap_identify(deviceName.constData(), CDDA_MESSAGE_LOGIT, &messages);
    if (messages != nullptr) {
        cdio_cddap_free_messages(messages);
        messages = nullptr;
    }

    if (drive == nullptr || cdio_cddap_open(drive) != 0) {
        if (drive != nullptr) {
            cdio_cddap_close(drive);
        }
        m_readerRunning.store(false);
        return;
    }

    cdrom_paranoia_t *paranoia = cdio_paranoia_init(drive);
    if (paranoia == nullptr) {
        cdio_cddap_close(drive);
        m_readerRunning.store(false);
        return;
    }

    cdio_paranoia_modeset(paranoia, PARANOIA_MODE_OVERLAP | PARANOIA_MODE_VERIFY);

    const qint64 startLba = discFirstLba();
    const qint64 endLba = discLastLba();
    const qint64 requestedStart = positionMsToAbsoluteLba(m_positionMs);
    const qint64 initialLba = qBound(startLba, requestedStart, endLba);
    qint64 currentLba = initialLba;

    cdio_paranoia_set_range(paranoia, static_cast<long>(startLba), static_cast<long>(endLba));
    cdio_paranoia_seek(paranoia, static_cast<int32_t>(currentLba), SEEK_SET);

    const int highWatermark = m_ringBuffer->capacity() * 3 / 4;
    const int lowWatermark = m_ringBuffer->capacity() / 2;
    const bool repeatAll = m_repeatAllEnabled.load();

    while (!m_readerStopRequested.load()) {
        if (m_ringBuffer->size() >= highWatermark) {
            QThread::msleep(5);
            continue;
        }

        if (currentLba > endLba) {
            if (repeatAll) {
                currentLba = startLba;
                cdio_paranoia_seek(paranoia, static_cast<int32_t>(currentLba), SEEK_SET);
            } else {
                QThread::msleep(5);
                continue;
            }
        }

        int sectorsRead = 0;
        while (sectorsRead < READER_CHUNK_SECTORS
               && !m_readerStopRequested.load()
               && m_ringBuffer->size() < highWatermark
               && currentLba <= endLba) {
            int16_t *frame = cdio_paranoia_read_limited(paranoia, nullptr, 20);
            if (frame == nullptr) {
                QThread::msleep(2);
                continue;
            }

            m_ringBuffer->write(reinterpret_cast<const char *>(frame), CD_BYTES_PER_SECTOR);
            ++currentLba;
            ++sectorsRead;
        }

        if (m_ringBuffer->size() < lowWatermark) {
            QThread::msleep(1);
        }
    }

    cdio_paranoia_free(paranoia);
    cdio_cddap_close(drive);
    m_readerRunning.store(false);
}

void CDNativePlaybackEngine::setDevicePath(const QString &devicePath)
{
    if (!devicePath.isEmpty()) {
        m_devicePath = devicePath;
    }
}

qint64 CDNativePlaybackEngine::positionMsToAbsoluteLba(qint64 positionMs) const
{
    if (m_tracks.isEmpty()) {
        return 0;
    }

    qint64 remaining = qMax<qint64>(0, positionMs);
    for (const CDNativeTrack &track : m_tracks) {
        if (remaining < track.durationMs) {
            const qint64 offsetLba = (remaining * 75) / 1000;
            const qint64 lastLbaInTrack = track.startLba + qMax<qint64>(0, track.lengthLba - 1);
            return qMin(track.startLba + offsetLba, lastLbaInTrack);
        }
        remaining -= track.durationMs;
    }

    return discLastLba();
}

qint64 CDNativePlaybackEngine::discFirstLba() const
{
    if (m_tracks.isEmpty()) {
        return 0;
    }
    return m_tracks.first().startLba;
}

qint64 CDNativePlaybackEngine::discLastLba() const
{
    if (m_tracks.isEmpty()) {
        return 0;
    }
    const CDNativeTrack &last = m_tracks.last();
    return last.startLba + qMax<qint64>(0, last.lengthLba - 1);
}

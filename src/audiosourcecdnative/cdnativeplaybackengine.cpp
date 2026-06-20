#include "cdnativeplaybackengine.h"

#include <cdio/paranoia/cdda.h>

#include <QMediaDevices>
#include <QRandomGenerator>
#include <QThread>
#include <QtConcurrent>

#include "cdpcmiodevice.h"
#include "cdpcmringbuffer.h"

namespace {
constexpr int CD_BYTES_PER_SECTOR = 2352;
constexpr int READER_CHUNK_SECTORS = 20;
constexpr int RING_BUFFER_SECTORS = 192;
constexpr int AUDIO_SINK_BUFFER_SECTORS = 32;
constexpr int TRANSITION_UNMUTE_DELAY_MS = 120;
constexpr int POSITION_TICK_MS = 100;
constexpr int INITIAL_PREROLL_BYTES = CD_BYTES_PER_SECTOR * 48;
constexpr int TARGET_BUFFER_BYTES = CD_BYTES_PER_SECTOR * 96;
constexpr int TRANSPORT_RETRY_MS = 10;
}

CDNativePlaybackEngine::CDNativePlaybackEngine(QObject *parent)
    : QObject(parent)
    , m_ringBuffer(new CDPcmRingBuffer(CD_BYTES_PER_SECTOR * RING_BUFFER_SECTORS))
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
    m_streamGeneration.fetch_add(1);
    m_transitionMutePending = true;

    m_tracks = tracks;
    m_positionMs = 0;
    m_positionAnchorMs = 0;
    m_canResumeFromPause = false;
    m_currentLogicalIndex = 0;
    m_currentOrderIndex = 0;
    if (m_transportStartPending) {
        m_transportStartPending = false;
        emit bufferingChanged(false);
    }

    qint64 total = 0;
    for (int i = 0; i < m_tracks.size(); ++i) {
        total += trackPlaybackDurationMs(i);
    }
    m_durationMs = total;

    rebuildPlayOrder();
    m_currentOrderIndex = findOrderIndexForTrack(m_currentLogicalIndex);
    emit durationChanged(m_durationMs);
    emit activeTrackChanged(resolveTrackIndexForPosition(m_positionMs));
    emit positionChanged(m_positionMs);
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
    if (m_positionTimer.isActive()) {
        seek(m_positionMs);
    }
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

    if (m_canResumeFromPause && m_audioSink != nullptr && m_audioSink->state() == QAudio::SuspendedState) {
        m_audioSink->resume();
        if (m_transportStartPending) {
            m_transportStartPending = false;
            emit bufferingChanged(false);
        }
        m_canResumeFromPause = false;
        m_positionTimer.start();
        return;
    }

    startTransportWhenReady();
}

void CDNativePlaybackEngine::pause()
{
    m_canResumeFromPause = true;

    if (m_transportStartPending) {
        m_transportStartPending = false;
        emit bufferingChanged(false);
    }

    if (m_audioSink != nullptr) {
        m_audioSink->suspend();
    }
    m_positionTimer.stop();
}

void CDNativePlaybackEngine::stop()
{
    m_streamGeneration.fetch_add(1);
    m_transitionMutePending = true;

    const int stopTrackIndex = resolveTrackIndexForPosition(m_positionMs);
    const qint64 stopTrackStartMs = stopTrackIndex >= 0 ? trackStartMs(stopTrackIndex) : 0;

    m_positionTimer.stop();
    m_positionMs = stopTrackStartMs;
    m_positionAnchorMs = stopTrackStartMs;
    m_canResumeFromPause = false;
    m_currentLogicalIndex = qMax(0, stopTrackIndex);

    recreateSink();

    stopReaderLoop(false);
    m_ringBuffer->clear();

    if (m_transportStartPending) {
        m_transportStartPending = false;
        emit bufferingChanged(false);
    }

    emit activeTrackChanged(m_currentLogicalIndex);
    emit positionChanged(m_positionMs);
}

void CDNativePlaybackEngine::prepareForEject()
{
    m_streamGeneration.fetch_add(1);
    m_transitionMutePending = true;

    m_positionTimer.stop();
    m_canResumeFromPause = false;

    recreateSink();

    stopReaderLoop(false);
    m_ringBuffer->clear();

    if (m_transportStartPending) {
        m_transportStartPending = false;
        emit bufferingChanged(false);
    }

    m_ejectPreparationPending = true;
    finishEjectPreparationWhenReady();
}

void CDNativePlaybackEngine::finishEjectPreparationWhenReady()
{
    if (!m_ejectPreparationPending) {
        return;
    }

    if (m_readerRunning.load()) {
        QTimer::singleShot(10, this, [this]() {
            finishEjectPreparationWhenReady();
        });
        return;
    }

    m_ejectPreparationPending = false;
    QTimer::singleShot(0, this, [this]() {
        emit ejectPreparationFinished();
    });
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

    int nextIndex;
    if (currentIndex >= m_tracks.size() - 1) {
        nextIndex = 0;
    } else {
        nextIndex = currentIndex + 1;
    }
    if (m_shuffleEnabled.load() && !m_playOrder.isEmpty()) {
        m_currentOrderIndex = findOrderIndexForTrack(currentIndex);
        const int lastOrderIndex = m_playOrder.size() - 1;
        if (m_currentOrderIndex >= lastOrderIndex) {
            m_currentOrderIndex = 0;
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

    m_streamGeneration.fetch_add(1);
    m_transitionMutePending = true;

    const qint64 clamped = qBound<qint64>(0, mseconds, m_durationMs);
    m_positionMs = clamped;
    m_canResumeFromPause = false;
    m_currentLogicalIndex = qMax(0, resolveTrackIndexForPosition(m_positionMs));
    m_currentOrderIndex = findOrderIndexForTrack(m_currentLogicalIndex);

    // Jump/seek is intentionally non-gapless. We reset transport to the new absolute position.
    const bool wasRunning = m_positionTimer.isActive();
    m_positionTimer.stop();

    if (m_transportStartPending) {
        m_transportStartPending = false;
        emit bufferingChanged(false);
    }

    if (m_audioSink != nullptr) {
        // Recreate sink to ensure backend output queue is fully reset.
        recreateSink();
    }

    stopReaderLoop(false);
    resetSinkStream();
    if (wasRunning) {
        startTransportWhenReady();
    }

    emit activeTrackChanged(m_currentLogicalIndex);
    emit positionChanged(m_positionMs);
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
    if (m_shuffleEnabled.load()) {
        return m_currentLogicalIndex;
    }
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
        const qint64 end = acc + trackPlaybackDurationMs(i);
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
        acc += trackPlaybackDurationMs(i);
    }
    return acc;
}

qint64 CDNativePlaybackEngine::trackPlaybackDurationMs(int index) const
{
    if (index < 0 || index >= m_tracks.size()) {
        return 0;
    }
    return (m_tracks.at(index).lengthLba * 1000) / 75;
}

qint64 CDNativePlaybackEngine::trackLastLba(int index) const
{
    if (index < 0 || index >= m_tracks.size()) {
        return discLastLba();
    }

    const CDNativeTrack &track = m_tracks.at(index);
    return track.startLba + qMax<qint64>(0, track.lengthLba - 1);
}

void CDNativePlaybackEngine::advanceToNextShuffleTrack()
{
    if (!m_shuffleEnabled.load() || m_playOrder.isEmpty()) {
        return;
    }

    const int nextOrderIndex = m_currentOrderIndex + 1;
    if (nextOrderIndex >= m_playOrder.size()) {
        if (m_repeatAllEnabled.load()) {
            m_currentOrderIndex = 0;
            rebuildPlayOrder();
        } else {
            finishNaturalPlayback();
            return;
        }
    } else {
        m_currentOrderIndex = nextOrderIndex;
    }

    const int nextTrack = m_playOrder.at(m_currentOrderIndex);
    const qint64 nextPos = trackStartMs(nextTrack);

    m_streamGeneration.fetch_add(1);
    m_transitionMutePending = true;
    m_positionMs = nextPos;
    m_positionAnchorMs = nextPos;
    m_canResumeFromPause = false;
    m_currentLogicalIndex = nextTrack;

    const bool wasRunning = m_positionTimer.isActive();
    m_positionTimer.stop();

    if (m_transportStartPending) {
        m_transportStartPending = false;
        emit bufferingChanged(false);
    }

    if (m_audioSink != nullptr) {
        recreateSink();
    }

    stopReaderLoop(false);
    resetSinkStream();
    if (wasRunning) {
        startTransportWhenReady();
    }

    emit activeTrackChanged(m_currentLogicalIndex);
    emit positionChanged(m_positionMs);
}

void CDNativePlaybackEngine::finishNaturalPlayback()
{
    m_positionTimer.stop();
    if (m_audioSink != nullptr) {
        m_audioSink->stop();
    }
    stopReaderLoop();

    m_streamGeneration.fetch_add(1);
    m_transitionMutePending = true;
    m_positionMs = 0;
    m_positionAnchorMs = 0;
    m_canResumeFromPause = false;
    m_currentLogicalIndex = 0;
    m_currentOrderIndex = findOrderIndexForTrack(0);

    recreateSink();
    m_ringBuffer->clear();

    if (m_transportStartPending) {
        m_transportStartPending = false;
        emit bufferingChanged(false);
    }

    emit activeTrackChanged(m_currentLogicalIndex);
    emit positionChanged(m_positionMs);
    emit playbackFinished();
}

void CDNativePlaybackEngine::updatePositionTick()
{
    if (m_audioSink != nullptr) {
        const qint64 processedMs = qMax<qint64>(0, m_audioSink->processedUSecs() / 1000);
        m_positionMs = qBound<qint64>(0, m_positionAnchorMs + processedMs, m_durationMs);
    }

    if (m_shuffleEnabled.load() && !m_tracks.isEmpty()) {
        const qint64 trackStart = trackStartMs(m_currentLogicalIndex);
        const qint64 trackEnd = trackStart + trackPlaybackDurationMs(m_currentLogicalIndex);
        m_positionMs = qMin(m_positionMs, trackEnd);

        const bool readerFinished = !m_readerRunning.load();
        const bool bufferDrained = m_ringBuffer->size() == 0;
        const bool trackPlaybackComplete = m_positionMs >= trackEnd
            || (bufferDrained && m_positionMs > trackStart);
        if (readerFinished && trackPlaybackComplete) {
            advanceToNextShuffleTrack();
            return;
        }
    } else if (m_positionMs >= m_durationMs) {
        if (m_repeatAllEnabled.load() && m_durationMs > 0) {
            m_positionMs = 0;
            m_positionAnchorMs = 0;
            m_positionTimer.stop();

            if (m_audioSink != nullptr) {
                m_audioSink->suspend();
            }

            stopReaderLoop(false);
            resetSinkStream();
            startTransportWhenReady();
        } else {
            finishNaturalPlayback();
            return;
        }
    }

    if (!m_shuffleEnabled.load()) {
        const int activeTrack = resolveTrackIndexForPosition(m_positionMs);
        if (activeTrack != m_currentLogicalIndex) {
            m_currentLogicalIndex = activeTrack;
            emit activeTrackChanged(m_currentLogicalIndex);
        }
    }

    emit positionChanged(m_positionMs);
}

void CDNativePlaybackEngine::ensureSink()
{
    if (m_audioSink != nullptr) {
        return;
    }

    m_audioSink = new QAudioSink(QMediaDevices::defaultAudioOutput(), m_audioFormat, this);
    m_audioSink->setBufferSize(CD_BYTES_PER_SECTOR * AUDIO_SINK_BUFFER_SECTORS);
    m_audioSink->setVolume(m_transitionMutePending ? 0.0f : 1.0f);
}

void CDNativePlaybackEngine::recreateSink()
{
    if (m_audioSink != nullptr) {
        m_audioSink->reset();
        m_audioSink->suspend();
        m_audioSink->deleteLater();
        m_audioSink = nullptr;
    }

    ensureSink();
}

void CDNativePlaybackEngine::resetSinkStream()
{
    m_ringBuffer->clear();
    recreateSink();
}

void CDNativePlaybackEngine::startTransportWhenReady()
{
    if (m_tracks.isEmpty()) {
        return;
    }

    if (!m_transportStartPending) {
        m_transportStartPending = true;
        emit bufferingChanged(true);
    }

    if (m_readerRunning.load() && m_readerStopRequested.load()) {
        QTimer::singleShot(TRANSPORT_RETRY_MS, this, [this]() {
            startTransportWhenReady();
        });
        return;
    }

    // Non-blocking stop requests may let the old reader write a bit more data
    // before it exits. Once it has actually stopped, clear again to prevent
    // stale PCM from bleeding into the next start/track.
    if (!m_readerRunning.load() && m_readerStopRequested.load()) {
        m_ringBuffer->clear();
    }

    startReaderLoop();

    if (m_ringBuffer->size() < INITIAL_PREROLL_BYTES) {
        if (m_readerRunning.load()) {
            QTimer::singleShot(TRANSPORT_RETRY_MS, this, [this]() {
                startTransportWhenReady();
            });
            return;
        }
        if (m_ringBuffer->size() == 0) {
            QTimer::singleShot(TRANSPORT_RETRY_MS, this, [this]() {
                startTransportWhenReady();
            });
            return;
        }
    }

    if (m_audioSink != nullptr && m_audioSink->state() != QAudio::ActiveState) {
        m_positionAnchorMs = m_positionMs;
        m_audioSink->start(m_pcmDevice);

        if (m_transitionMutePending) {
            QTimer::singleShot(TRANSITION_UNMUTE_DELAY_MS, this, [this]() {
                if (m_audioSink != nullptr) {
                    m_audioSink->setVolume(1.0f);
                }
                m_transitionMutePending = false;
            });
        }
    }

    if (m_transportStartPending) {
        m_transportStartPending = false;
        emit bufferingChanged(false);
    }

    if (!m_positionTimer.isActive()) {
        m_positionTimer.start();
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

void CDNativePlaybackEngine::stopReaderLoop(bool waitForFinish)
{
    if (!m_readerRunning.load()) {
        return;
    }

    m_readerStopRequested.store(true);

    if (!waitForFinish) {
        return;
    }

    m_readerFuture.waitForFinished();
    m_readerRunning.store(false);
}

void CDNativePlaybackEngine::readerLoop()
{
    const uint64_t streamGeneration = m_streamGeneration.load();

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

    // Keep the drive at a conservative speed to avoid aggressive spin-up/down.
    cdio_cddap_speed_set(drive, 4);

    const qint64 startLba = discFirstLba();
    const int currentTrackIndex = m_currentLogicalIndex;
    const bool shuffle = m_shuffleEnabled.load();
    const qint64 endLba = shuffle ? trackLastLba(currentTrackIndex) : discLastLba();
    const qint64 requestedStart = positionMsToAbsoluteLba(m_positionMs);
    const qint64 initialLba = qBound(startLba, requestedStart, endLba);
    qint64 currentLba = initialLba;

    QByteArray readBuffer(READER_CHUNK_SECTORS * CD_BYTES_PER_SECTOR, Qt::Uninitialized);
    const bool repeatAll = m_repeatAllEnabled.load() && !shuffle;

    while (!m_readerStopRequested.load() && streamGeneration == m_streamGeneration.load()) {
        if (m_ringBuffer->size() >= TARGET_BUFFER_BYTES) {
            QThread::msleep(3);
            continue;
        }

        if (currentLba > endLba) {
            if (shuffle) {
                break;
            }
            if (repeatAll) {
                currentLba = startLba;
            } else {
                QThread::msleep(5);
                continue;
            }
        }

        const qint64 remainingSectors = endLba - currentLba + 1;
        if (remainingSectors <= 0) {
            QThread::msleep(2);
            continue;
        }

        const int writableBytes = m_ringBuffer->capacity() - m_ringBuffer->size();
        const int writableSectors = writableBytes / CD_BYTES_PER_SECTOR;
        const int sectorsToRead = qMin(READER_CHUNK_SECTORS,
                                       qMin(static_cast<int>(remainingSectors), writableSectors));

        if (sectorsToRead <= 0) {
            QThread::msleep(2);
            continue;
        }

        const long sectorsRead = cdio_cddap_read(drive,
                                                 readBuffer.data(),
                                                 static_cast<lsn_t>(currentLba),
                                                 static_cast<long>(sectorsToRead));
        if (sectorsRead <= 0) {
            // Skip ahead over bad/slow sectors similarly to VLC's resilient behavior.
            ++currentLba;
            QThread::msleep(1);
            continue;
        }

        const int bytesRead = static_cast<int>(sectorsRead) * CD_BYTES_PER_SECTOR;
        if (streamGeneration != m_streamGeneration.load()) {
            break;
        }

        const int bytesWritten = m_ringBuffer->write(readBuffer.constData(), bytesRead);
        if (bytesWritten > 0) {
            m_pcmDevice->notifyReadyRead();
        }
        currentLba += sectorsRead;
    }

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
    for (int i = 0; i < m_tracks.size(); ++i) {
        const CDNativeTrack &track = m_tracks.at(i);
        const qint64 trackDurationMs = trackPlaybackDurationMs(i);
        if (remaining < trackDurationMs) {
            const qint64 offsetLba = (remaining * 75) / 1000;
            const qint64 lastLbaInTrack = track.startLba + qMax<qint64>(0, track.lengthLba - 1);
            return qMin(track.startLba + offsetLba, lastLbaInTrack);
        }
        remaining -= trackDurationMs;
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

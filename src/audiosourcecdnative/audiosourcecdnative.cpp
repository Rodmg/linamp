#include "audiosourcecdnative.h"

#include <QMediaMetaData>

#include "cdnativeplaybackengine.h"

#include <QtConcurrent>

AudioSourceCDNative::AudioSourceCDNative(QObject *parent)
    : AudioSourceWSpectrumCapture(parent)
    , m_discService("/dev/cdrom")
    , m_engine(new CDNativePlaybackEngine(this))
{
    m_metadataService.setDevicePath("/dev/cdrom");
    m_engine->setDevicePath("/dev/cdrom");

    m_detectDiscTimer.setInterval(5000);
    connect(&m_detectDiscTimer, &QTimer::timeout, this, &AudioSourceCDNative::pollDisc);
    m_detectDiscTimer.start();

    connect(m_engine, &CDNativePlaybackEngine::positionChanged, this, [this](qint64 absolutePositionMs) {
        const int trackIndex = m_engine->currentTrackIndex();
        if (trackIndex < 0) {
            emit positionChanged(0);
            return;
        }
        const qint64 relativePosition = qMax<qint64>(0, absolutePositionMs - trackStartMs(trackIndex));
        emit positionChanged(relativePosition);
    });
    connect(m_engine, &CDNativePlaybackEngine::durationChanged, this, [this](qint64) {
        const int trackIndex = m_lastTrackIndex >= 0 ? m_lastTrackIndex : 0;
        if (trackIndex >= 0 && trackIndex < m_tracks.size()) {
            emit durationChanged(m_tracks.at(trackIndex).durationMs);
        }
    });
    connect(m_engine, &CDNativePlaybackEngine::activeTrackChanged, this, [this](int index) {
        emitTrackMetadata(index);
    });
    connect(m_engine, &CDNativePlaybackEngine::ejectPreparationFinished, this, [this]() {
        finalizeEject();
    });
    connect(m_engine, &CDNativePlaybackEngine::bufferingChanged, this, [this](bool buffering) {
        if (!m_discLoaded || m_ejectInProgress) {
            return;
        }

        if (buffering) {
            emit messageSet("LOADING...", 1500);
        } else {
            emit messageClear();
        }
    });
    connect(m_engine, &CDNativePlaybackEngine::playbackFinished, this, [this]() {
        emit playbackStateChanged(MediaPlayer::StoppedState);
        if (m_isActive) {
            stopSpectrum();
        }
    });
    connect(&m_ejectWatcher, &QFutureWatcher<bool>::finished, this, &AudioSourceCDNative::completeEject);
}

AudioSourceCDNative::~AudioSourceCDNative() = default;

void AudioSourceCDNative::activate()
{
    m_isActive = true;
    emit eqEnabledChanged(false);
    emit plEnabledChanged(false);
    emit shuffleEnabledChanged(m_shuffleEnabled);
    emit repeatEnabledChanged(m_repeatEnabled);
    emit playbackStateChanged(MediaPlayer::StoppedState);

    pollDisc();

    if (m_discLoaded) {
        m_lastTrackIndex = -1;
        const int index = qMax(0, m_engine->currentTrackIndex());
        emit positionChanged(0);
        emitTrackMetadata(index);
    } else {
        emit positionChanged(0);
        emit durationChanged(0);
        emitNoDiscMetadata();
    }
}

void AudioSourceCDNative::deactivate()
{
    m_isActive = false;
    stopSpectrum();
    m_engine->stop();
    emit playbackStateChanged(MediaPlayer::StoppedState);
}

void AudioSourceCDNative::handlePl()
{
    emit plEnabledChanged(false);
}

void AudioSourceCDNative::handlePrevious()
{
    if (!m_discLoaded) {
        return;
    }
    m_engine->previous();
}

void AudioSourceCDNative::handlePlay()
{
    if (!m_discLoaded) {
        return;
    }

    m_engine->play();
    emit playbackStateChanged(MediaPlayer::PlayingState);
    if (m_isActive) {
        startSpectrum();
    }
}

void AudioSourceCDNative::handlePause()
{
    if (!m_discLoaded) {
        return;
    }

    m_engine->pause();
    emit playbackStateChanged(MediaPlayer::PausedState);
    if (m_isActive) {
        stopSpectrum();
    }
}

void AudioSourceCDNative::handleStop()
{
    m_engine->stop();
    emit playbackStateChanged(MediaPlayer::StoppedState);
    emit positionChanged(0);
    if (m_isActive) {
        stopSpectrum();
    }
}

void AudioSourceCDNative::handleNext()
{
    if (!m_discLoaded) {
        return;
    }
    m_engine->next();
}

void AudioSourceCDNative::handleOpen()
{
    if (m_ejectInProgress || m_ejectWatcher.isRunning()) {
        return;
    }

    if (!m_discService.isDiscInserted()) {
        emit messageSet("NO DISC", 3000);
        return;
    }

    m_ejectInProgress = true;
    emit messageSet("EJECTING...", 4000);
    m_engine->prepareForEject();
}

void AudioSourceCDNative::finalizeEject()
{
    if (!m_ejectInProgress) {
        return;
    }

    QFuture<bool> future = QtConcurrent::run([this]() {
        return m_discService.eject();
    });
    m_ejectWatcher.setFuture(future);
}

void AudioSourceCDNative::completeEject()
{
    if (!m_ejectInProgress) {
        return;
    }

    const bool ejected = m_ejectWatcher.result();
    m_ejectInProgress = false;

    if (!ejected) {
        emit messageSet("EJECT FAILED", 3000);
        return;
    }

    unloadDisc();
    emit messageClear();
}

void AudioSourceCDNative::handleShuffle()
{
    m_shuffleEnabled = !m_shuffleEnabled;
    m_engine->setShuffleEnabled(m_shuffleEnabled);
    emit shuffleEnabledChanged(m_shuffleEnabled);
}

void AudioSourceCDNative::handleRepeat()
{
    m_repeatEnabled = !m_repeatEnabled;
    m_engine->setRepeatAllEnabled(m_repeatEnabled);
    emit repeatEnabledChanged(m_repeatEnabled);
}

void AudioSourceCDNative::handleSeek(int mseconds)
{
    if (!m_discLoaded) {
        return;
    }

    const int activeTrack = m_engine->currentTrackIndex();
    if (activeTrack < 0 || activeTrack >= m_tracks.size()) {
        return;
    }

    const qint64 clampedRelative = qBound<qint64>(0, static_cast<qint64>(mseconds),
                                                   trackPlaybackDurationMs(activeTrack));
    m_engine->seek(trackStartMs(activeTrack) + clampedRelative);
}

void AudioSourceCDNative::pollDisc()
{
    const bool inserted = m_discService.isDiscInserted();

    if (inserted && !m_discLoaded) {
        emit requestActivation();
        loadDisc();
    } else if (!inserted && m_discLoaded) {
        unloadDisc();
    }
}

void AudioSourceCDNative::loadDisc()
{
    emit messageSet("LOADING...", 3000);

    const QList<CDNativeTrack> layout = m_discService.readTrackLayout();
    if (layout.isEmpty()) {
        unloadDisc();
        emit messageSet("NO DISC", 3000);
        return;
    }

    QList<CDNativeTrack> filtered;
    for (const CDNativeTrack &track : layout) {
        if (!track.isDataTrack) {
            filtered.append(track);
        }
    }

    m_tracks = m_metadataService.enrich(filtered);
    m_engine->setTracks(m_tracks);

    m_discLoaded = !m_tracks.isEmpty();
    m_lastTrackIndex = -1;

    if (!m_discLoaded) {
        unloadDisc();
        emit messageSet("NO AUDIO TRACKS", 3000);
        return;
    }

    emit playbackStateChanged(MediaPlayer::StoppedState);
    emit positionChanged(0);
    emitTrackMetadata(0);
    emit messageClear();
}

void AudioSourceCDNative::unloadDisc()
{
    m_engine->stop();
    m_tracks.clear();
    m_discLoaded = false;
    m_lastTrackIndex = -1;

    emit playbackStateChanged(MediaPlayer::StoppedState);
    emit positionChanged(0);
    emit durationChanged(0);
    emitNoDiscMetadata();

    if (m_isActive) {
        stopSpectrum();
    }
}

void AudioSourceCDNative::emitNoDiscMetadata()
{
    QMediaMetaData metadata;
    metadata.insert(QMediaMetaData::Title, "NO DISC");
    emit metadataChanged(metadata);
}

void AudioSourceCDNative::emitTrackMetadata(int index)
{
    if (index < 0 || index >= m_tracks.size()) {
        return;
    }

    if (index == m_lastTrackIndex) {
        return;
    }

    const CDNativeTrack track = m_tracks.at(index);
    m_lastTrackIndex = index;

    QMediaMetaData metadata;
    metadata.insert(QMediaMetaData::Title, track.title);
    metadata.insert(QMediaMetaData::AlbumArtist, track.artist);
    metadata.insert(QMediaMetaData::AlbumTitle, track.album);
    metadata.insert(QMediaMetaData::TrackNumber, track.trackNumber);
    metadata.insert(QMediaMetaData::Duration, track.durationMs);
    metadata.insert(QMediaMetaData::AudioBitRate, 1411 * 1000);
    metadata.insert(QMediaMetaData::Comment, "44100");

    emit durationChanged(track.durationMs);
    emit metadataChanged(metadata);
}

int AudioSourceCDNative::resolveTrackIndexForAbsolutePosition(qint64 absoluteMs) const
{
    if (m_tracks.isEmpty()) {
        return -1;
    }

    qint64 elapsed = 0;
    for (int i = 0; i < m_tracks.size(); ++i) {
        const qint64 trackDuration = trackPlaybackDurationMs(i);
        const qint64 trackEnd = elapsed + trackDuration;
        if (absoluteMs < trackEnd || i == m_tracks.size() - 1) {
            return i;
        }
        elapsed = trackEnd;
    }

    return m_tracks.size() - 1;
}

qint64 AudioSourceCDNative::trackPlaybackDurationMs(int index) const
{
    if (index < 0 || index >= m_tracks.size()) {
        return 0;
    }
    return (m_tracks.at(index).lengthLba * 1000) / 75;
}

qint64 AudioSourceCDNative::trackStartMs(int index) const
{
    if (index <= 0) {
        return 0;
    }

    qint64 elapsed = 0;
    for (int i = 0; i < index && i < m_tracks.size(); ++i) {
        elapsed += trackPlaybackDurationMs(i);
    }
    return elapsed;
}

#include "audiosourcecdnative.h"

#include <QMediaMetaData>

#include "cdnativeplaybackengine.h"

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

    connect(m_engine, &CDNativePlaybackEngine::positionChanged, this, &AudioSourceCDNative::positionChanged);
    connect(m_engine, &CDNativePlaybackEngine::durationChanged, this, &AudioSourceCDNative::durationChanged);
    connect(m_engine, &CDNativePlaybackEngine::activeTrackChanged, this, [this](int index) {
        emitTrackMetadata(index);
    });
}

AudioSourceCDNative::~AudioSourceCDNative() = default;

void AudioSourceCDNative::activate()
{
    m_isActive = true;
    emit eqEnabledChanged(false);
    emit plEnabledChanged(false);
    emit shuffleEnabledChanged(m_shuffleEnabled);
    emit repeatEnabledChanged(m_repeatEnabled);

    pollDisc();
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
    emit messageSet("EJECTING...", 4000);

    const bool ejected = m_discService.eject();
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
    m_engine->seek(mseconds);
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

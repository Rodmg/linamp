#include <QTime>
#include <QFileDialog>

#include "audiosourcefile.h"
#include "util.h"

//#define DISPLAY_BUFFERING


AudioSourceFile::AudioSourceFile(QObject *parent, PlaylistModel *playlistModel)
    : AudioSource{parent}
{
    m_player = new MediaPlayer(this);

    connect(m_player, &MediaPlayer::durationChanged, this, &AudioSourceFile::durationChanged);
    connect(m_player, &MediaPlayer::positionChanged, this, &AudioSourceFile::positionChanged);
    connect(m_player, QOverload<>::of(&MediaPlayer::metaDataChanged), this,
            &AudioSourceFile::handleMetaDataChanged);
    connect(m_player, &MediaPlayer::mediaStatusChanged, this, &AudioSourceFile::handleMediaStatusChanged);
    connect(m_player, &MediaPlayer::bufferProgressChanged, this, &AudioSourceFile::handleBufferingProgress);
    connect(m_player, &MediaPlayer::errorChanged, this, &AudioSourceFile::handleMediaError);

    m_playlistModel = playlistModel;
    m_playlist = m_playlistModel->playlist();

    connect(m_playlist, &QMediaPlaylist::currentIndexChanged, this,
            &AudioSourceFile::handlePlaylistPositionChanged);
    connect(m_playlist, &QMediaPlaylist::mediaAboutToBeRemoved, this,
            &AudioSourceFile::handlePlaylistMediaRemoved);

    connect(m_player, &MediaPlayer::playbackStateChanged, this, &AudioSourceFile::playbackStateChanged);

    // Emit data for spectrum analyzer
    connect(m_player, &MediaPlayer::newData, this, &AudioSourceFile::handleSpectrumData);

}

void AudioSourceFile::activate()
{
    emit playbackStateChanged(m_player->playbackState());
    emit positionChanged(m_player->position());
    emit metadataChanged(m_player->metaData());
    emit durationChanged(m_player->duration());
    emit eqEnabledChanged(false);
    emit plEnabledChanged(true);
    emit shuffleEnabledChanged(shuffleEnabled);
    emit repeatEnabledChanged(repeatEnabled);
}

void AudioSourceFile::deactivate()
{
    // Stop everything
    shouldBePlaying = false;
    m_player->stop();
}

void AudioSourceFile::handlePl()
{
    emit showPlaylistRequested();
    emit plEnabledChanged(true);
}

void AudioSourceFile::handlePrevious()
{
    // Go to previous track if we are within the first 5 seconds of playback
    // Otherwise, seek to the beginning.
    if (m_player->position() <= 5000) {
        // If is first item in playlist, do nothing
        // That's handled by QMediaPlaylist
        m_playlist->previous();
    } else {
        m_player->setPosition(0);
    }
}

void AudioSourceFile::handlePlay()
{
    shouldBePlaying = true;
    m_player->play();
}

void AudioSourceFile::handlePause()
{
    shouldBePlaying = false;
    m_player->pause();
}

void AudioSourceFile::handleStop()
{
    shouldBePlaying = false;
    m_player->stop();
}

void AudioSourceFile::handleNext()
{
    // If is last item in playlist:
    // If repeat enabled, go to first
    // If not, do nothing
    // That's handled by QMediaPlaylist
    m_playlist->next();
}

void AudioSourceFile::handleOpen()
{
    emit showPlaylistRequested();
}

void AudioSourceFile::handleShuffle()
{
    shuffleEnabled = !shuffleEnabled;
    m_playlist->setShuffle(shuffleEnabled);
    emit shuffleEnabledChanged(shuffleEnabled);
}

void AudioSourceFile::handleRepeat()
{
    repeatEnabled = !repeatEnabled;
    QMediaPlaylist::PlaybackMode mode = QMediaPlaylist::PlaybackMode::Sequential;
    if(repeatEnabled) mode = QMediaPlaylist::PlaybackMode::Loop;
    m_playlist->setPlaybackMode(mode);
    emit repeatEnabledChanged(repeatEnabled);
}

void AudioSourceFile::handleSeek(int mseconds)
{
    m_player->setPosition(mseconds);

}

void AudioSourceFile::handleMetaDataChanged()
{
    auto metaData = m_player->metaData();
    emit metadataChanged(metaData);
}

void AudioSourceFile::handleMediaStatusChanged(MediaPlayer::MediaStatus status)
{
    // handle status message
    switch (status) {
    case MediaPlayer::NoMedia:
    case MediaPlayer::LoadedMedia:
        setStatusInfo(QString());
        break;
    case MediaPlayer::LoadingMedia:
        setStatusInfo(tr("Loading..."));
        break;
    case MediaPlayer::BufferingMedia:
    case MediaPlayer::BufferedMedia:
        #ifdef DISPLAY_BUFFERING
        setStatusInfo(tr("Buffering %1%").arg(qRound(m_player->bufferProgress())));
        #endif
        break;
    case MediaPlayer::StalledMedia:
        setStatusInfo(tr("Stalled %1%").arg(qRound(m_player->bufferProgress())));
        break;
    case MediaPlayer::EndOfMedia:
        handleNext();
        break;
    case MediaPlayer::InvalidMedia:
        handleMediaError();
        break;
    }
}

void AudioSourceFile::handleBufferingProgress(float progress)
{

    if (m_player->mediaStatus() == MediaPlayer::StalledMedia) {
        setStatusInfo(tr("Stalled %1%").arg(qRound(progress)));
    }
    else {
        #ifdef DISPLAY_BUFFERING
        setStatusInfo(tr("Buffering %1%").arg(qRound(progress)));
        #endif
    }
}

void AudioSourceFile::handleMediaError()
{
    if (m_player->error() == MediaPlayer::NoError)
        return;
    setStatusInfo(m_player->errorString());
    qDebug() << m_player->errorString();
}

void AudioSourceFile::setStatusInfo(const QString &info)
{
    m_statusInfo = info;

    if (!m_statusInfo.isEmpty())
        emit messageSet(QString("%1").arg(m_statusInfo), 1000);
    else
        emit messageClear();
}

void AudioSourceFile::handlePlaylistPositionChanged(int)
{
    m_player->setSource(m_playlist->currentQueueMedia());

    if (shouldBePlaying) {
        m_player->play();
    }
}

void AudioSourceFile::handlePlaylistMediaRemoved(int from, int to)
{
    // Stop only if currently playing file was removed
    int playingIndex = m_playlist->currentIndex();
    if(playingIndex >= from && playingIndex <= to) {
        shouldBePlaying = false;
        m_player->stop();
        m_player->clearSource();
    }
}

void AudioSourceFile::jump(const QModelIndex &index)
{
    if (index.isValid()) {
        m_playlist->setCurrentIndex(index.row());
        shouldBePlaying = true;
        m_player->play();
    }
}

void AudioSourceFile::addToPlaylist(const QList<QUrl> &urls)
{
    const int previousMediaCount = m_playlist->mediaCount();
    for (auto &url : urls) {
        if (isPlaylist(url))
            m_playlist->load(url);
        else
            m_playlist->addMedia(url);
    }
    if (m_playlist->mediaCount() > previousMediaCount) {
        // Start playing only if not already playing
        if(m_player->playbackState() == MediaPlayer::PlaybackState::StoppedState) {
            auto index = m_playlistModel->index(previousMediaCount, 0);
            jump(index);
        }
    }
}

void AudioSourceFile::handleSpectrumData(const QByteArray& data)
{
    emit dataEmitted(data, m_player->format());
}

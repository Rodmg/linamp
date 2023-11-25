#include <QTime>
#include <QApplication>

#include "audiosourcefile.h"
#include "util.h"


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
            &PlayerView::playlistPositionChanged);
    connect(m_playlist, &QMediaPlaylist::mediaAboutToBeRemoved, this,
            &PlayerView::playlistMediaRemoved);

    connect(m_player, &MediaPlayer::playbackStateChanged, this, &AudioSourceFile::playbackStateChanged);

    // Emit data for spectrum analyzer
    connect(m_player, &MediaPlayer::newData, this, &AudioSourceFile::dataEmitted);

}

void AudioSourceFile::activate()
{
    emit playbackStateChanged(m_player->playbackState());
    emit positionChanged(0);
    // emit metadataChanged(...) TODO get current playlist file meta
    // emit durationChanged(...)
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
        setStatusInfo(tr("Buffering %1%").arg(qRound(m_player->bufferProgress() * 100.)));
        break;
    case MediaPlayer::StalledMedia:
        setStatusInfo(tr("Stalled %1%").arg(qRound(m_player->bufferProgress() * 100.)));
        break;
    case MediaPlayer::EndOfMedia:
        //QApplication::alert(this);
        handleNext();
        break;
    case MediaPlayer::InvalidMedia:
        handleMediaError();
        break;
    }
}

void AudioSourceFile::handleBufferingProgress(float progress)
{

    if (m_player->mediaStatus() == MediaPlayer::StalledMedia)
        setStatusInfo(tr("Stalled %1%").arg(qRound(progress * 100.)));
    else
        setStatusInfo(tr("Buffering %1%").arg(qRound(progress * 100.)));
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
        emit messageSet(QString("%1 | %2").arg(m_statusInfo), 5000);
    else
        emit messageClear();
}

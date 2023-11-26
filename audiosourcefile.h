#ifndef AUDIOSOURCEFILE_H
#define AUDIOSOURCEFILE_H

#include <QObject>
#include <QAudioOutput>

#include "audiosource.h"
#include "qmediaplaylist.h"
#include "playlistmodel.h"
#include "mediaplayer.h"

class AudioSourceFile : public AudioSource
{
    Q_OBJECT
public:
    explicit AudioSourceFile(QObject *parent = nullptr, PlaylistModel *playlistModel = nullptr);

signals:
    void showPlaylistRequested();

public slots:
    void activate();
    void deactivate();
    void handlePl();
    void handlePrevious();
    void handlePlay();
    void handlePause();
    void handleStop();
    void handleNext();
    void handleOpen();
    void handleShuffle();
    void handleRepeat();
    void handleSeek(int mseconds);

    void jump(const QModelIndex &index);

    void addToPlaylist(const QList<QUrl> &urls);

private slots:
    void handleMetaDataChanged();
    void handleMediaStatusChanged(MediaPlayer::MediaStatus status);
    void handleBufferingProgress(float progress);
    void handleMediaError();
    void handlePlaylistPositionChanged(int);
    void handlePlaylistMediaRemoved(int, int);
    void handleSpectrumData(const QByteArray& data);


private:
    MediaPlayer *m_player = nullptr;
    QMediaPlaylist *m_playlist = nullptr;
    PlaylistModel *m_playlistModel = nullptr;

    QString m_statusInfo;

    bool shuffleEnabled = false;
    bool repeatEnabled = false;
    bool shouldBePlaying = false;

    void setStatusInfo(const QString &info);

};

#endif // AUDIOSOURCEFILE_H

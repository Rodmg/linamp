#ifndef AUDIOSOURCE_H
#define AUDIOSOURCE_H

#include <QObject>
#include "mediaplayer.h"


class AudioSource : public QObject
{
    Q_OBJECT
public:
    explicit AudioSource(QObject *parent = nullptr);

signals:
    void playbackStateChanged(MediaPlayer::PlaybackState state);
    void positionChanged(qint64 progress);
    void dataEmitted(const QByteArray& data);
    void metadataChanged(QMediaMetaData metadata);
    void durationChanged(qint64 duration);
    void eqEnabledChanged(bool enabled);
    void plEnabledChanged(bool enabled);
    void shuffleEnabledChanged(bool enabled);
    void repeatEnabledChanged(bool enabled);
    void messageSet(QString message, qint64 timeout);
    void messageClear();

public slots:
    virtual void activate() = 0;
    virtual void deactivate() = 0;
    virtual void handlePl() = 0;
    virtual void handlePrevious() = 0;
    virtual void handlePlay() = 0;
    virtual void handlePause() = 0;
    virtual void handleStop() = 0;
    virtual void handleNext() = 0;
    virtual void handleOpen() = 0;
    virtual void handleShuffle() = 0;
    virtual void handleRepeat() = 0;
    virtual void handleSeek(int mseconds) = 0;

};

#endif // AUDIOSOURCE_H

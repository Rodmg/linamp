#ifndef AUDIOSOURCECOORDINATOR_H
#define AUDIOSOURCECOORDINATOR_H

#include <QObject>
#include "audiosource.h"
#include "systemaudiocontrol.h"
#include "playerview.h"

class AudioSourceCoordinator : public QObject
{
    Q_OBJECT
public:
    explicit AudioSourceCoordinator(QObject *parent = nullptr, PlayerView *playerView = nullptr);

signals:
    void sourceChanged(int source);

public slots:
    void setSource(int source);
    void setVolume(int volume);
    void setBalance(int balance);

private:
    QList<AudioSource*> sources;
    int currentSource = 0;

    SystemAudioControl *system_audio = nullptr;
    PlayerView *view = nullptr;
};

#endif // AUDIOSOURCECOORDINATOR_H

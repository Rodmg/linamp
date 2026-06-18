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

    void addSource(AudioSource *source, QString label, bool activate = false);

signals:
    void sourceChanged(int source);

public slots:
    void setSource(int source);
    void setVolume(int volume);
    void setBalance(int balance);

private slots:
    void onVolumeDragStarted();
    void onVolumeDragFinished(int volume);
    void onBalanceDragStarted();
    void onBalanceDragFinished(int balance);

private:
    QList<AudioSource*> sources;
    QList<QString> sourceLabels;
    int currentSource = -1;

    SystemAudioControl *system_audio = nullptr;
    PlayerView *view = nullptr;

    bool m_volumeDragging = false;
    bool m_balanceDragging = false;

    void showVolumeMessage(int volume, bool persistent);
    void showBalanceMessage(int balance, bool persistent);
};

#endif // AUDIOSOURCECOORDINATOR_H

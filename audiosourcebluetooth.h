#ifndef AUDIOSOURCEBLUETOOTH_H
#define AUDIOSOURCEBLUETOOTH_H

#include <QObject>

#include "audiosource.h"

class AudioSourceBluetooth : public AudioSource
{
    Q_OBJECT
public:
    explicit AudioSourceBluetooth(QObject *parent = nullptr);

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
};

#endif // AUDIOSOURCEBLUETOOTH_H

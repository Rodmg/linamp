#ifndef AUDIOSOURCEBLUETOOTH_H
#define AUDIOSOURCEBLUETOOTH_H

#include <QObject>
#include <QTimer>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

#include "audiosource.h"

#define SERVICE_NAME "org.bluez"
#define OBJ_PATH "/org/bluez/hci0/dev_5C_70_17_02_D7_6E/player2"
#define OBJ_INTERFACE "org.bluez.MediaPlayer1"

class AudioSourceBluetooth : public AudioSource
{
    Q_OBJECT
public:
    explicit AudioSourceBluetooth(QObject *parent = nullptr);
    ~AudioSourceBluetooth();

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

private:
    QTimer *dataEmitTimer = nullptr;
    void emitData();

    QDBusInterface *dbusIface = nullptr;

    bool isShuffleEnabled = false;
    bool isRepeatEnabled = false;

private slots:
    void btStatusChanged(QString name, QVariantMap map, QStringList list);
};

#endif // AUDIOSOURCEBLUETOOTH_H

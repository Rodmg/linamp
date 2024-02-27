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

    QTimer *progressTimer = nullptr;
    void refreshProgress();

    bool isShuffleEnabled = false;
    bool isRepeatEnabled = false;

    void startSpectrum();
    void stopSpectrum();

    void fetchBtMetadata();


private slots:
    void handleBtPropertyChange(QString name, QVariantMap map, QStringList list);
    void handleBtStatusChange(QString status);
    void handleBtTrackChange(QVariantMap trackData);
    void handleBtShuffleChange(QString shuffleSetting);
    void handleBtRepeatChange(QString repeatSetting);
    void handleBtPositionChange(quint32 position);
};

#endif // AUDIOSOURCEBLUETOOTH_H

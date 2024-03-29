#ifndef AUDIOSOURCEBLUETOOTH_H
#define AUDIOSOURCEBLUETOOTH_H

#include <QObject>
#include <QTimer>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusMetaType>

#include "audiosource.h"

#define SERVICE_NAME "org.bluez"
#define OBJ_PATH "/org/bluez/hci0/dev_5C_70_17_02_D7_6E/player2"
#define OBJ_INTERFACE "org.bluez.MediaPlayer1"

class BluezMediaInterface : public QDBusAbstractInterface
{
    Q_OBJECT

public:
    static inline const char *staticInterfaceName()
    { return OBJ_INTERFACE; }

public:
    BluezMediaInterface(const QString &service, const QString &path, const QDBusConnection &connection,
                    QObject *parent = nullptr):
        QDBusAbstractInterface(service, path, staticInterfaceName(), connection, parent) {}

    virtual ~BluezMediaInterface() {}

    Q_PROPERTY(QVariantMap Track READ track)
    QVariantMap track() const
    {
        return qvariant_cast<QVariantMap>(property("Track"));
    }

    Q_PROPERTY(QString Status READ status)
    QString status() const
    {
        return qvariant_cast<QString>(property("Status"));
    }

    Q_PROPERTY(QString Repeat READ repeat WRITE setRepeat)
    QString repeat() const
    {
        return qvariant_cast<QString>(property("Repeat"));
    }
    void setRepeat(QString value)
    {
        setProperty("Repeat", value);
    }

    Q_PROPERTY(QString Shuffle READ shuffle WRITE setShuffle)
    QString shuffle() const
    {
        return qvariant_cast<QString>(property("Shuffle"));
    }
    void setShuffle(QString value)
    {
        setProperty("Shuffle", value);
    }

    Q_PROPERTY(quint32 Position READ position)
    quint32 position() const
    {
        return qvariant_cast<quint32>(property("Position"));
    }

Q_SIGNALS:
    void PropertiesChanged(const QVariantMap &properties);
};

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

    BluezMediaInterface *dbusIface = nullptr;

    QTimer *progressRefreshTimer = nullptr;
    QTimer *progressInterpolateTimer = nullptr;
    quint32 currentProgress = 0;
    void refreshProgress();
    void interpolateProgress();

    bool isShuffleEnabled = false;
    bool isRepeatEnabled = false;

    void startSpectrum();
    void stopSpectrum();

    void fetchBtMetadata();

    // DBus connection utils
    QString findDbusMediaObjPath(); // returns empty string if not found
    bool setupDbusIface();
    bool isDbusReady();
    void dbusCall(QString method);


private slots:
    void handleBtPropertyChange(QString name, QVariantMap map, QStringList list);
    void handleBtStatusChange(QString status);
    void handleBtTrackChange(QVariantMap trackData);
    void handleBtShuffleChange(QString shuffleSetting);
    void handleBtRepeatChange(QString repeatSetting);
    void handleBtPositionChange(quint32 position);
};

#endif // AUDIOSOURCEBLUETOOTH_H

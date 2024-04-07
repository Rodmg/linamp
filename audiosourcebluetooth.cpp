#include "audiosourcebluetooth.h"

AudioSourceBluetooth::AudioSourceBluetooth(QObject *parent)
    : AudioSourceWSpectrumCapture{parent}
{
    setupDbusIface();

    // Track progress with timer
    progressRefreshTimer = new QTimer(this);
    progressRefreshTimer->setInterval(1000);
    connect(progressRefreshTimer, &QTimer::timeout, this, &AudioSourceBluetooth::refreshProgress);
    progressInterpolateTimer = new QTimer(this);
    progressInterpolateTimer->setInterval(33);
    connect(progressInterpolateTimer, &QTimer::timeout, this, &AudioSourceBluetooth::interpolateProgress);
}

AudioSourceBluetooth::~AudioSourceBluetooth()
{
}

void AudioSourceBluetooth::handleBtStatusChange(QString status)
{
    if(status == "playing") {
        startSpectrum();
        progressRefreshTimer->start();
        progressInterpolateTimer->start();
        emit this->playbackStateChanged(MediaPlayer::PlayingState);
    }
    if(status == "stopped") {
        stopSpectrum();
        progressRefreshTimer->stop();
        progressInterpolateTimer->stop();
        emit this->playbackStateChanged(MediaPlayer::StoppedState);
    }
    if(status == "paused") {
        stopSpectrum();
        progressRefreshTimer->stop();
        progressInterpolateTimer->stop();
        emit this->playbackStateChanged(MediaPlayer::PausedState);
    }
    if(status == "error") {
        stopSpectrum();
        progressRefreshTimer->stop();
        progressInterpolateTimer->stop();
        emit this->playbackStateChanged(MediaPlayer::StoppedState);
        emit this->messageSet("Bluetooth Error", 5000);
    }
}

void AudioSourceBluetooth::handleBtTrackChange(QVariantMap trackData)
{
    QMediaMetaData metadata;
    for( QString trackKey : trackData.keys()){
        qDebug() << "    " << trackKey << ":" << trackData.value(trackKey);
        if(trackKey == "Title") {
            metadata.insert(QMediaMetaData::Title, trackData.value(trackKey));
        }
        if(trackKey == "Artist") {
            metadata.insert(QMediaMetaData::AlbumArtist, trackData.value(trackKey));
        }
        if(trackKey == "Album") {
            metadata.insert(QMediaMetaData::AlbumTitle, trackData.value(trackKey));
        }
        if(trackKey == "Genre") {
            metadata.insert(QMediaMetaData::Genre, trackData.value(trackKey));
        }
        if(trackKey == "TrackNumber") {
            metadata.insert(QMediaMetaData::TrackNumber, trackData.value(trackKey));
        }
        if(trackKey == "Duration") {
            quint32 duration = trackData.value(trackKey).toUInt();
            metadata.insert(QMediaMetaData::Duration, duration);
            emit this->durationChanged(duration);
        }
    }
    emit this->metadataChanged(metadata);
}

void AudioSourceBluetooth::handleBtShuffleChange(QString shuffleSetting)
{
    if(shuffleSetting == "off") {
        emit this->shuffleEnabledChanged(false);
        this->isShuffleEnabled = false;
    } else {
        emit this->shuffleEnabledChanged(true);
        this->isShuffleEnabled = true;
    }
}

void AudioSourceBluetooth::handleBtRepeatChange(QString repeatSetting)
{
    if(repeatSetting == "off") {
        emit this->repeatEnabledChanged(false);
        this->isRepeatEnabled = false;
    } else {
        emit this->repeatEnabledChanged(true);
        this->isRepeatEnabled = true;
    }
}

void AudioSourceBluetooth::handleBtPositionChange(quint32 position)
{
    this->currentProgress = position;
    emit this->positionChanged(position);
}

void AudioSourceBluetooth::handleBtPropertyChange(QString name, QVariantMap map, QStringList list)
{
    qDebug() << QString("properties of interface %1 changed").arg(name);
    for (QVariantMap::const_iterator it = map.cbegin(), end = map.cend(); it != end; ++it) {
        qDebug() << "property: " << it.key() << " value: " << it.value();

        QString prop = it.key();

        if(prop == "Status") {
            QString status = it.value().toString();
            this->handleBtStatusChange(status);
        }

        if(prop == "Track") {
            QVariantMap trackData = qdbus_cast<QVariantMap>(it.value());
            this->handleBtTrackChange(trackData);
        }

        if(prop == "Repeat") {
            QString repeatSetting = it.value().toString();
            this->handleBtRepeatChange(repeatSetting);
        }

        if(prop == "Shuffle") {
            QString shuffleSetting = it.value().toString();
            this->handleBtShuffleChange(shuffleSetting);
        }

        if(prop == "Position") {
            quint32 pos = it.value().toUInt();
            this->handleBtPositionChange(pos);
        }

    }
    for (const auto& element : list) {
        qDebug() << "list element: " << element;
    }
}

void AudioSourceBluetooth::fetchBtMetadata()
{
    if(!isDbusReady()) {
        return;
    }

    QVariant status = this->dbusIface->property("Status");
    if(!status.isNull()) {
        qDebug() << "<<<<Status:" << status.toString();
        this->handleBtStatusChange(status.toString());
    }

    QVariant repeat = this->dbusIface->property("Repeat");
    if(!repeat.isNull()) {
        qDebug() << "<<<<Repeat:" << repeat.toString();
        this->handleBtRepeatChange(repeat.toString());
    }

    QVariant shuffle = this->dbusIface->property("Shuffle");
    if(!shuffle.isNull()) {
        qDebug() << "<<<<Shuffle:" << shuffle.toString();
        this->handleBtShuffleChange(shuffle.toString());
    }

    QVariant track = this->dbusIface->property("Track");
    if(!track.isNull()) {
        qDebug() << "<<<<Track:";
        QVariantMap trackData = qdbus_cast<QVariantMap>(track);
        this->handleBtTrackChange(trackData);
    }

    this->refreshProgress();
}

void AudioSourceBluetooth::activate()
{
    QMediaMetaData metadata = QMediaMetaData{};
    metadata.insert(QMediaMetaData::Title, "Bluetooth");

    emit playbackStateChanged(MediaPlayer::StoppedState);
    emit positionChanged(0);
    emit metadataChanged(metadata);
    emit durationChanged(0);
    emit eqEnabledChanged(false);
    emit plEnabledChanged(false);
    emit shuffleEnabledChanged(false);
    emit repeatEnabledChanged(false);

    this->isShuffleEnabled = false;
    this->isRepeatEnabled = false;

    fetchBtMetadata();
}

void AudioSourceBluetooth::deactivate()
{
    stopSpectrum();
    emit playbackStateChanged(MediaPlayer::StoppedState);
}

void AudioSourceBluetooth::handlePl()
{
    emit plEnabledChanged(false);
}

void AudioSourceBluetooth::handlePrevious()
{
    dbusCall("Previous");
}

void AudioSourceBluetooth::handlePlay()
{
    //startSpectrum();
    progressRefreshTimer->start();
    progressInterpolateTimer->start();
    dbusCall("Play");
    emit playbackStateChanged(MediaPlayer::PlayingState);
    fetchBtMetadata();
}

void AudioSourceBluetooth::handlePause()
{
    stopSpectrum();
    progressRefreshTimer->stop();
    progressInterpolateTimer->stop();
    dbusCall("Pause");
    emit playbackStateChanged(MediaPlayer::PausedState);
}

void AudioSourceBluetooth::handleStop()
{
    stopSpectrum();
    progressRefreshTimer->stop();
    progressInterpolateTimer->stop();
    dbusCall("Stop");
    emit playbackStateChanged(MediaPlayer::StoppedState);
}

void AudioSourceBluetooth::handleNext()
{
    dbusCall("Next");
}

void AudioSourceBluetooth::handleOpen()
{

}

void AudioSourceBluetooth::handleShuffle()
{
    this->isShuffleEnabled = !this->isShuffleEnabled;
    emit shuffleEnabledChanged(this->isShuffleEnabled);
    if(isDbusReady()) {
        this->dbusIface->setShuffle(this->isShuffleEnabled ? "alltracks" : "off");
    }
}

void AudioSourceBluetooth::handleRepeat()
{
    this->isRepeatEnabled = !this->isRepeatEnabled;
    emit repeatEnabledChanged(this->isRepeatEnabled);
    if(isDbusReady()) {
        this->dbusIface->setRepeat(this->isRepeatEnabled ? "alltracks" : "off");
    }
}

void AudioSourceBluetooth::handleSeek(int mseconds)
{

}

void AudioSourceBluetooth::refreshProgress()
{
    QVariant position = this->dbusIface->property("Position");
    if(!position.isNull()) {
        this->handleBtPositionChange(position.toUInt());
    }
}

void AudioSourceBluetooth::interpolateProgress()
{
    this->currentProgress += 33;
    emit this->positionChanged(this->currentProgress);
}

QString AudioSourceBluetooth::findDbusMediaObjPath()
{

}

bool AudioSourceBluetooth::setupDbusIface()
{
    // Init dbus
    auto dbusConn = QDBusConnection::systemBus();
    if(!dbusConn.isConnected()) {
        qDebug() << "Cannot connect to DBuss";
        return false;
    } else {
        qDebug() << "CONNECTED!";
    }

    this->dbusIface = new BluezMediaInterface(SERVICE_NAME, OBJ_PATH, dbusConn, this);
    if(!this->dbusIface->isValid()) {
        qDebug() << "DBus interface is invalid";
        return false;
    }

    bool success = dbusConn.connect(SERVICE_NAME, OBJ_PATH, "org.freedesktop.DBus.Properties", "PropertiesChanged", this, SLOT(handleBtPropertyChange(QString, QVariantMap, QStringList)));

    if(success) {
        qDebug() << "SUCCESS!";
    } else {
        qDebug() << "MEH";
    }
    return success;
}

bool AudioSourceBluetooth::isDbusReady()
{
    if(this->dbusIface == nullptr) {
        return false;
    }
    return this->dbusIface->isValid();
}

void AudioSourceBluetooth::dbusCall(QString method)
{
    if(isDbusReady()) {
        this->dbusIface->call(method);
    }
}

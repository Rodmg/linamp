#include "audiosourcebluetooth.h"
#include "spectrumwidget.h"
#include "util.h"
#include "systemaudiocapture_pulseaudio.h"

#include <QtConcurrent>

extern QMutex *sampleMutex;
extern QByteArray *sample;
extern QDataStream *sampleStream;

AudioSourceBluetooth::AudioSourceBluetooth(QObject *parent)
    : AudioSource{parent}
{
    sampleMutex = new QMutex();
    sample = new QByteArray();
    sampleStream = new QDataStream(sample, QIODevice::WriteOnly);
    dataEmitTimer = new QTimer(this);
    dataEmitTimer->setInterval(33); // around 30 fps
    connect(dataEmitTimer, &QTimer::timeout, this, &AudioSourceBluetooth::emitData);

    // Init dbus
    qDBusRegisterMetaType<QVariantMap>();
    auto dbusConn = QDBusConnection::systemBus();
    if(!dbusConn.isConnected()) {
        qDebug() << "Cannot connect to DBuss";
        return;
    } else {
        qDebug() << "CONNECTED!";
    }

    this->dbusIface = new BluezMediaInterface(SERVICE_NAME, OBJ_PATH, dbusConn, this);
    if(!this->dbusIface->isValid()) {
        qDebug() << "DBus interface is invalid";
        return;
    }

    bool success = dbusConn.connect(SERVICE_NAME, OBJ_PATH, "org.freedesktop.DBus.Properties", "PropertiesChanged", this, SLOT(handleBtPropertyChange(QString, QVariantMap, QStringList)));

    if(success) {
        qDebug() << "SUCCESS!";
    } else {
        qDebug() << "MEH";
    }

    // Track progress with timer
    progressTimer = new QTimer(this);
    progressTimer->setInterval(1000);
    connect(progressTimer, &QTimer::timeout, this, &AudioSourceBluetooth::refreshProgress);
}

AudioSourceBluetooth::~AudioSourceBluetooth()
{
    stopPACapture();
}

void AudioSourceBluetooth::handleBtStatusChange(QString status)
{
    if(status == "playing") {
        startSpectrum();
        progressTimer->start();
        emit this->playbackStateChanged(MediaPlayer::PlayingState);
    }
    if(status == "stopped") {
        stopSpectrum();
        progressTimer->stop();
        emit this->playbackStateChanged(MediaPlayer::StoppedState);
    }
    if(status == "paused") {
        stopSpectrum();
        progressTimer->stop();
        emit this->playbackStateChanged(MediaPlayer::PausedState);
    }
    if(status == "error") {
        stopSpectrum();
        progressTimer->stop();
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
    if(!this->dbusIface->isValid()) {
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


void AudioSourceBluetooth::startSpectrum()
{
    QtConcurrent::run(startPACapture);
    dataEmitTimer->start();
}

void AudioSourceBluetooth::stopSpectrum()
{
    stopPACapture();
    dataEmitTimer->stop();
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
    if(this->dbusIface->isValid()) {
        this->dbusIface->call("Previous");
    }
}

void AudioSourceBluetooth::handlePlay()
{
    startSpectrum();
    progressTimer->start();
    if(this->dbusIface->isValid()) {
        this->dbusIface->call("Play");
    }
    emit playbackStateChanged(MediaPlayer::PlayingState);
    fetchBtMetadata();
}

void AudioSourceBluetooth::handlePause()
{
    stopSpectrum();
    progressTimer->stop();
    if(this->dbusIface->isValid()) {
        this->dbusIface->call("Pause");
    }
    emit playbackStateChanged(MediaPlayer::PausedState);
}

void AudioSourceBluetooth::handleStop()
{
    stopSpectrum();
    progressTimer->stop();
    if(this->dbusIface->isValid()) {
        this->dbusIface->call("Stop");
    }
    emit playbackStateChanged(MediaPlayer::StoppedState);
}

void AudioSourceBluetooth::handleNext()
{
    if(this->dbusIface->isValid()) {
        this->dbusIface->call("Next");
    }
}

void AudioSourceBluetooth::handleOpen()
{

}

void AudioSourceBluetooth::handleShuffle()
{
    this->isShuffleEnabled = !this->isShuffleEnabled;
    emit shuffleEnabledChanged(this->isShuffleEnabled);
    if(this->dbusIface->isValid()) {
        this->dbusIface->setShuffle(this->isShuffleEnabled ? "alltracks" : "off");
    }
}

void AudioSourceBluetooth::handleRepeat()
{
    this->isRepeatEnabled = !this->isRepeatEnabled;
    emit repeatEnabledChanged(this->isRepeatEnabled);
    if(this->dbusIface->isValid()) {
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


void AudioSourceBluetooth::emitData()
{
    QMutexLocker l(sampleMutex);

    if(sample->length() < DFT_SIZE * 4) {
        return;
    }

    if(!isValidSample(sample)) {
        return;
    }

    //dbg
    /*QString dbgsample = "";
    for(int i = 0; i < 100; i++) {
        dbgsample += QString(sample->at(i)) + ",";
    }
    qDebug() << ">>>>>" << dbgsample;
    */
    // end dbg

    QAudioFormat format;
    format.setSampleFormat(QAudioFormat::Int16);
    format.setSampleRate(PA_SAMPLE_RATE);
    format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    format.setChannelCount(PA_CHANNELS);
    emit dataEmitted(*sample, format);
    sample->clear();
    delete sampleStream;
    sampleStream = new QDataStream(sample, QIODevice::WriteOnly);
}

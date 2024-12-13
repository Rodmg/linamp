#include "audiosourcebluetooth.h"

//#define DEBUG_ASPY

#define ASPY_PROGRESS_INTERPOLATION_TIME 100

AudioSourceBluetooth::AudioSourceBluetooth(QObject *parent)
    : AudioSourceWSpectrumCapture{parent}
{
    auto state = PyGILState_Ensure();

    // Import 'linamp' python module, see python folder in the root of this repo
    PyObject *pModuleName = PyUnicode_DecodeFSDefault("linamp");
    //PyObject *pModuleName = PyUnicode_DecodeFSDefault("linamp-mock");
    playerModule = PyImport_Import(pModuleName);
    Py_DECREF(pModuleName);

    if(playerModule == nullptr) {
        qDebug() << "Couldn't load python module";
        PyGILState_Release(state);
        return;
    }

    PyObject *PlayerClass = PyObject_GetAttrString(playerModule, "BTPlayer");

    if(!PlayerClass || !PyCallable_Check(PlayerClass)) {
        qDebug() << "Error getting Player Class";
        PyGILState_Release(state);
        return;
    }

    player = PyObject_CallNoArgs(PlayerClass);

    // Timer to detect changes and load
    detectChangesTimer = new QTimer(this);
    detectChangesTimer->setInterval(1000);
    connect(detectChangesTimer, &QTimer::timeout, this, &AudioSourceBluetooth::pollDetectChanges);
    detectChangesTimer->start();

    // Watch for async change detection results
    connect(&pollResultWatcher, &QFutureWatcher<bool>::finished, this, &AudioSourceBluetooth::handlePollResult);

    // Handle load end
    connect(&loadWatcher, &QFutureWatcher<bool>::finished, this, &AudioSourceBluetooth::handleLoadEnd);

    // Handle finish ejecting
    connect(&ejectWatcher, &QFutureWatcher<bool>::finished, this, &AudioSourceBluetooth::handleEjectEnd);


    // Track progress with timer
    progressRefreshTimer = new QTimer(this);
    progressRefreshTimer->setInterval(1000);
    connect(progressRefreshTimer, &QTimer::timeout, this, &AudioSourceBluetooth::refreshProgress);
    progressInterpolateTimer = new QTimer(this);
    progressInterpolateTimer->setInterval(ASPY_PROGRESS_INTERPOLATION_TIME);
    connect(progressInterpolateTimer, &QTimer::timeout, this, &AudioSourceBluetooth::interpolateProgress);

    PyGILState_Release(state);
}

AudioSourceBluetooth::~AudioSourceBluetooth()
{
}

void AudioSourceBluetooth::pollDetectChanges()
{
    if(pollResultWatcher.isRunning() || loadWatcher.isRunning()) {
        #ifdef DEBUG_ASPY
        qDebug() << ">>>>>>>>>>>>>>>POLL Avoided";
        #endif
        return;
    }
    if(pollInProgress) return;
    pollInProgress = true;
    #ifdef DEBUG_ASPY
    qDebug() << "pollDetectChanges: polling";
    #endif
    QFuture<bool> status = QtConcurrent::run(&AudioSourceBluetooth::doPollDetectChanges, this);
    //pollStatus = &status;
    pollResultWatcher.setFuture(status);
}

void AudioSourceBluetooth::handlePollResult()
{
    #ifdef DEBUG_ASPY
    qDebug() << ">>>>POLL RESULT";
    #endif

    bool changeDetected = pollResultWatcher.result();
    if(changeDetected) {
        if(loadWatcher.isRunning()) {
            #ifdef DEBUG_ASPY
            qDebug() << ">>>>>>>>>>>>>>>LOAD Avoided";
            #endif
            return;
        }
        //emit this->requestActivation(); // Request audiosource coordinator to select us
        QFuture<void> status = QtConcurrent::run(&AudioSourceBluetooth::doLoad, this);
        loadWatcher.setFuture(status);
    } else {
        pollInProgress = false;
    }
}

bool AudioSourceBluetooth::doPollDetectChanges()
{
    bool changeDetected = false;
    if(player == nullptr) return changeDetected;

    auto state = PyGILState_Ensure();
    PyObject* pyChangeDetected = PyObject_CallMethod(player, "poll_changes", NULL);

    if(PyBool_Check(pyChangeDetected)) {
        changeDetected = PyObject_IsTrue(pyChangeDetected);
        #ifdef DEBUG_ASPY
        qDebug() << ">>>Change detected?:" << changeDetected;
        #endif
    } else {
        #ifdef DEBUG_ASPY
        qDebug() << ">>>>pollDetectChanges: Not a bool";
        #endif
    }
    if(pyChangeDetected) Py_DECREF(pyChangeDetected);
    PyGILState_Release(state);
    return changeDetected;
}

void AudioSourceBluetooth::doLoad()
{
    auto state = PyGILState_Ensure();
    PyObject_CallMethod(player, "load", NULL);
    PyGILState_Release(state);
}

void AudioSourceBluetooth::handleLoadEnd()
{
    emit this->messageClear();
    refreshStatus();
    pollInProgress = false;
}

void AudioSourceBluetooth::doEject()
{
    auto state = PyGILState_Ensure();
    PyObject_CallMethod(player, "eject", NULL);
    PyGILState_Release(state);
}

void AudioSourceBluetooth::handleEjectEnd()
{
    emit this->messageClear();
    currentTrackNumber = std::numeric_limits<quint32>::max();
    refreshStatus();
}


void AudioSourceBluetooth::activate()
{
    emit playbackStateChanged(MediaPlayer::StoppedState);
    emit positionChanged(0);
    emit durationChanged(0);
    emit eqEnabledChanged(false);
    emit plEnabledChanged(false);
    emit shuffleEnabledChanged(false);
    emit repeatEnabledChanged(false);

    refreshStatus();
    refreshTrackInfo(true);
    // Poll status
    progressRefreshTimer->start();

    isActive = true;
}

void AudioSourceBluetooth::deactivate()
{
    isActive = false;

    progressRefreshTimer->stop();
    stopSpectrum();
    emit playbackStateChanged(MediaPlayer::StoppedState);
    if(player == nullptr) return;
    auto state = PyGILState_Ensure();
    PyObject_CallMethod(player, "stop", NULL);
    PyGILState_Release(state);

}

void AudioSourceBluetooth::handlePl()
{
    emit plEnabledChanged(false);
}

void AudioSourceBluetooth::handlePrevious()
{
    if(player == nullptr) return;
    auto state = PyGILState_Ensure();
    PyObject *pyResult = PyObject_CallMethod(player, "prev", NULL);
    if(pyResult == nullptr) {
        PyErr_Print();
    }
    PyGILState_Release(state);
    refreshStatus();
}

void AudioSourceBluetooth::handlePlay()
{
    if(player == nullptr) return;
    auto state = PyGILState_Ensure();
    PyObject *pyResult = PyObject_CallMethod(player, "play", NULL);
    if(pyResult == nullptr) {
        PyErr_Print();
    }
    PyGILState_Release(state);
    refreshStatus();
}

void AudioSourceBluetooth::handlePause()
{
    if(player == nullptr) return;
    auto state = PyGILState_Ensure();
    PyObject *pyResult = PyObject_CallMethod(player, "pause", NULL);
    if(pyResult == nullptr) {
        PyErr_Print();
    }
    PyGILState_Release(state);
    refreshStatus();
}

void AudioSourceBluetooth::handleStop()
{
    if(player == nullptr) return;
    auto state = PyGILState_Ensure();
    PyObject *pyResult = PyObject_CallMethod(player, "stop", NULL);
    if(pyResult == nullptr) {
        PyErr_Print();
    }
    PyGILState_Release(state);
    refreshStatus();
}

void AudioSourceBluetooth::handleNext()
{
    if(player == nullptr) return;
    auto state = PyGILState_Ensure();
    PyObject *pyResult = PyObject_CallMethod(player, "next", NULL);
    if(pyResult == nullptr) {
        PyErr_Print();
    }
    PyGILState_Release(state);
    refreshStatus();
}

void AudioSourceBluetooth::handleOpen()
{
    if(player == nullptr) return;
    #ifdef DEBUG_ASPY
    qDebug() << "<<<<<EJECTING";
    #endif
    if(ejectWatcher.isRunning()) {
        #ifdef DEBUG_ASPY
        qDebug() << ">>>>>>>>>>>>>>>EJECT Avoided";
        #endif
        return;
    }
    emit this->messageSet("EJECTING...", 4000);
    QFuture<void> status = QtConcurrent::run(&AudioSourceBluetooth::doEject, this);
    ejectWatcher.setFuture(status);
}

void AudioSourceBluetooth::handleShuffle()
{
    if(player == nullptr) return;

    this->isShuffleEnabled = !this->isShuffleEnabled;

    auto state = PyGILState_Ensure();
    PyObject_CallMethod(player, "set_shuffle", "i", this->isShuffleEnabled);
    PyGILState_Release(state);

    refreshStatus(false);
}

void AudioSourceBluetooth::handleRepeat()
{
    if(player == nullptr) return;

    this->isRepeatEnabled = !this->isRepeatEnabled;

    auto state = PyGILState_Ensure();
    PyObject_CallMethod(player, "set_repeat", "i", this->isRepeatEnabled);
    PyGILState_Release(state);

    refreshStatus(false);
}

void AudioSourceBluetooth::handleSeek(int mseconds)
{
    if(player == nullptr) return;

    auto state = PyGILState_Ensure();
    PyObject_CallMethod(player, "seek", "l", mseconds);
    PyGILState_Release(state);

    refreshStatus(false);
}

void AudioSourceBluetooth::refreshStatus(bool shouldRefreshTrackInfo)
{
    if(player == nullptr) return;
    auto state = PyGILState_Ensure();

    // Get shuffle status
    PyObject *pyShuffleEnabled = PyObject_CallMethod(player, "get_shuffle", NULL);
    if(PyBool_Check(pyShuffleEnabled)) {
        this->isShuffleEnabled = PyObject_IsTrue(pyShuffleEnabled);
        emit shuffleEnabledChanged(this->isShuffleEnabled);
    }
    Py_DECREF(pyShuffleEnabled);

    // Get repeat status
    PyObject *pyRepeatEnabled = PyObject_CallMethod(player, "get_repeat", NULL);
    if(PyBool_Check(pyRepeatEnabled)) {
        this->isRepeatEnabled = PyObject_IsTrue(pyRepeatEnabled);
        emit repeatEnabledChanged(this->isRepeatEnabled);
    }
    Py_DECREF(pyRepeatEnabled);

    PyObject *pyStatus = PyObject_CallMethod(player, "get_status", NULL);
    if(pyStatus == nullptr) {
        PyGILState_Release(state);
        return;
    }
    QString status(PyUnicode_AsUTF8(pyStatus));
    Py_DECREF(pyStatus);

    PyGILState_Release(state);

    #ifdef DEBUG_ASPY
    qDebug() << ">>>Status" << status;
    #endif

    if(status == "disconnected") {
        QMediaMetaData metadata = QMediaMetaData{};
        metadata.insert(QMediaMetaData::Title, "DISCONNECTED");
        emit metadataChanged(metadata);
        emit playbackStateChanged(MediaPlayer::StoppedState);
        emit positionChanged(0);
        emit durationChanged(0);

        if(isActive) {
            progressInterpolateTimer->stop();
            stopSpectrum();
        }
    }

    if(status == "stopped") {
        emit playbackStateChanged(MediaPlayer::StoppedState);
        emit positionChanged(0);

        if(isActive) {
            progressInterpolateTimer->stop();
            stopSpectrum();
        }
    }

    if(status == "playing") {
        emit this->messageClear();
        emit playbackStateChanged(MediaPlayer::PlayingState);

        if(isActive) {
            progressInterpolateTimer->start();
            startSpectrum();
        }
    }

    if(status == "paused") {
        emit this->messageClear();
        emit playbackStateChanged(MediaPlayer::PausedState);

        if(isActive) {
            progressInterpolateTimer->stop();
            stopSpectrum();
        }
    }

    if(status == "loading") {
        emit playbackStateChanged(MediaPlayer::StoppedState);
        emit positionChanged(0);

        if(isActive) {
            progressInterpolateTimer->stop();
            stopSpectrum();
        }

        emit this->messageSet("LOADING...", 3000);
    }

    if(status == "error") {
        emit playbackStateChanged(MediaPlayer::StoppedState);
        emit positionChanged(0);

        if(isActive) {
            progressInterpolateTimer->stop();
            stopSpectrum();
        }

        emit this->messageSet("BT ERROR", 5000);
    }

    this->currentStatus = status;

    if(status != "disconnected" && shouldRefreshTrackInfo) {
        refreshTrackInfo();
    }
}

void AudioSourceBluetooth::refreshTrackInfo(bool force)
{
    #ifdef DEBUG_ASPY
    qDebug() << ">>>>>>>>>Refresh track info";
    #endif
    if(player == nullptr) return;
    auto state = PyGILState_Ensure();
    PyObject *pyTrackInfo = PyObject_CallMethod(player, "get_current_track_info", NULL);
    if(pyTrackInfo == nullptr) {
        #ifdef DEBUG_ASPY
        qDebug() << ">>> Couldn't get track info";
        #endif
        PyErr_Print();
        PyGILState_Release(state);
        return;
    }
    // format (tracknumber: int, artist, album, title, duration: int, is_data_track: bool)
    PyObject *pyTrackNumber = PyTuple_GetItem(pyTrackInfo, 0);
    PyObject *pyArtist = PyTuple_GetItem(pyTrackInfo, 1);
    PyObject *pyAlbum = PyTuple_GetItem(pyTrackInfo, 2);
    PyObject *pyTitle = PyTuple_GetItem(pyTrackInfo, 3);
    PyObject *pyDuration = PyTuple_GetItem(pyTrackInfo, 4);

    quint32 trackNumber = PyLong_AsLong(pyTrackNumber);

    if(trackNumber == this->currentTrackNumber && !force) {
        // No need to refresh
        Py_DECREF(pyTrackInfo);
        PyGILState_Release(state);
        return;
    }

    QString artist(PyUnicode_AsUTF8(pyArtist));
    QString album(PyUnicode_AsUTF8(pyAlbum));
    QString title(PyUnicode_AsUTF8(pyTitle));
    quint32 duration = PyLong_AsLong(pyDuration);

    QMediaMetaData metadata;
    metadata.insert(QMediaMetaData::Title, title);
    metadata.insert(QMediaMetaData::AlbumArtist, artist);
    metadata.insert(QMediaMetaData::AlbumTitle, album);
    metadata.insert(QMediaMetaData::TrackNumber, trackNumber);
    metadata.insert(QMediaMetaData::Duration, duration);
    metadata.insert(QMediaMetaData::AudioBitRate, 1411 * 1000);
    metadata.insert(QMediaMetaData::AudioCodec, 44100); // Using AudioCodec as sample rate for now

    this->currentTrackNumber = trackNumber;
    emit this->durationChanged(duration);
    emit this->metadataChanged(metadata);

    #ifdef DEBUG_ASPY
    qDebug() << ">>>>>>>>METADATA changed";
    #endif

    Py_DECREF(pyTrackInfo);

    PyGILState_Release(state);

}

void AudioSourceBluetooth::refreshProgress()
{
    if(player == nullptr) return;

    refreshStatus();

    auto state = PyGILState_Ensure();

    PyObject *pyPosition = PyObject_CallMethod(player, "get_postition", NULL);
    if(pyPosition == nullptr) {
        #ifdef DEBUG_ASPY
        qDebug() << ">>> Couldn't get track position";
        #endif
        PyErr_Print();
        PyGILState_Release(state);
        return;
    }
    if(PyLong_Check(pyPosition)) {
        quint32 position = PyLong_AsLong(pyPosition);

        int diff = (int)this->currentProgress - (int)position;
        #ifdef DEBUG_ASPY
        qDebug() << ">>>>Time diff" << diff;
        #endif

        // Avoid small jumps caused by the python method latency
        if(abs(diff) > 1000) {
            this->currentProgress = position;
            emit this->positionChanged(this->currentProgress);
        }
    }
    Py_DECREF(pyPosition);

    PyGILState_Release(state);
}

void AudioSourceBluetooth::interpolateProgress()
{
    if(!progressInterpolateElapsedTimer.isValid()) {
        // Handle first time
        progressInterpolateElapsedTimer.start();
        return;
    }
    qint64 elapsed = progressInterpolateElapsedTimer.elapsed();
    if(elapsed > 200) {
        // Handle invalid interpolations
        progressInterpolateElapsedTimer.start();
        return;
    }
    this->currentProgress += elapsed;
    progressInterpolateElapsedTimer.start();
    emit this->positionChanged(this->currentProgress);
}

#include "audiosourcepython.h"

//#define DEBUG_ASPY

#define ASPY_PROGRESS_INTERPOLATION_TIME 100

AudioSourcePython::AudioSourcePython(QString module, QString className, QObject *parent)
    : AudioSourceWSpectrumCapture{parent}
{
    auto state = PyGILState_Ensure();

    // Import specified python module
    PyObject *pModuleName = PyUnicode_DecodeFSDefault(module.toStdString().c_str());
    playerModule = PyImport_Import(pModuleName);
    Py_DECREF(pModuleName);

    if(playerModule == nullptr) {
        qDebug() << "Couldn't load python module";
        PyGILState_Release(state);
        return;
    }

    PyObject *PlayerClass = PyObject_GetAttrString(playerModule, className.toStdString().c_str());

    if(!PlayerClass || !PyCallable_Check(PlayerClass)) {
        qDebug() << "Error getting Player Class";
        PyGILState_Release(state);
        return;
    }

    player = PyObject_CallNoArgs(PlayerClass);
    if(player == nullptr) {
        PyErr_Print();
    }

    // Timer to poll for events and load
    pollEventsTimer = new QTimer(this);
    pollEventsTimer->setInterval(1000);
    connect(pollEventsTimer, &QTimer::timeout, this, &AudioSourcePython::pollEvents);
    pollEventsTimer->start();

    // Watch for async events poll results
    connect(&pollResultWatcher, &QFutureWatcher<bool>::finished, this, &AudioSourcePython::handlePollResult);

    // Handle load end
    connect(&loadWatcher, &QFutureWatcher<bool>::finished, this, &AudioSourcePython::handleLoadEnd);

    // Handle finish ejecting
    connect(&ejectWatcher, &QFutureWatcher<bool>::finished, this, &AudioSourcePython::handleEjectEnd);


    // Track progress with timer
    progressRefreshTimer = new QTimer(this);
    progressRefreshTimer->setInterval(1000);
    connect(progressRefreshTimer, &QTimer::timeout, this, &AudioSourcePython::refreshProgress);
    progressInterpolateTimer = new QTimer(this);
    progressInterpolateTimer->setInterval(ASPY_PROGRESS_INTERPOLATION_TIME);
    connect(progressInterpolateTimer, &QTimer::timeout, this, &AudioSourcePython::interpolateProgress);

    PyGILState_Release(state);

    QtConcurrent::run(&AudioSourcePython::runPythonLoop, this);
}

AudioSourcePython::~AudioSourcePython()
{
}

void AudioSourcePython::pollEvents()
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
    qDebug() << "pollEvents: polling";
    #endif
    QFuture<bool> status = QtConcurrent::run(&AudioSourcePython::doPollEvents, this);
    pollResultWatcher.setFuture(status);
}

void AudioSourcePython::handlePollResult()
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
        emit this->requestActivation(); // Request audiosource coordinator to select us
        QFuture<void> status = QtConcurrent::run(&AudioSourcePython::doLoad, this);
        loadWatcher.setFuture(status);
    } else {
        refreshStatus();
        pollInProgress = false;
    }

    // Handle messages
    this->refreshMessage();
}

bool AudioSourcePython::doPollEvents()
{
    bool changeDetected = false;
    if(player == nullptr) return changeDetected;

    auto state = PyGILState_Ensure();
    PyObject* pyChangeDetected = PyObject_CallMethod(player, "poll_events", NULL);

    if(pyChangeDetected != nullptr && PyBool_Check(pyChangeDetected)) {
        changeDetected = PyObject_IsTrue(pyChangeDetected);
        #ifdef DEBUG_ASPY
        qDebug() << ">>>Change detected?:" << changeDetected;
        #endif
    } else {
        #ifdef DEBUG_ASPY
        qDebug() << ">>>>pollEvents: Not a bool";
        #endif
    }
    if(pyChangeDetected) Py_DECREF(pyChangeDetected);
    PyGILState_Release(state);
    return changeDetected;
}

void AudioSourcePython::doLoad()
{
    auto state = PyGILState_Ensure();
    PyObject_CallMethod(player, "load", NULL);
    PyGILState_Release(state);
}

void AudioSourcePython::handleLoadEnd()
{
    emit this->messageClear();
    refreshStatus();
    pollInProgress = false;
}

void AudioSourcePython::doEject()
{
    auto state = PyGILState_Ensure();
    PyObject_CallMethod(player, "eject", NULL);
    PyGILState_Release(state);
}

void AudioSourcePython::handleEjectEnd()
{
    emit this->messageClear();
    // Empty metadata
    QMediaMetaData metadata;
    currentMetadata = metadata;
    refreshStatus();
}


void AudioSourcePython::activate()
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

void AudioSourcePython::deactivate()
{
    isActive = false;

    progressRefreshTimer->stop();
    stopSpectrum();
    emit playbackStateChanged(MediaPlayer::StoppedState);
    emit this->messageClear();
    if(player == nullptr) return;
    auto state = PyGILState_Ensure();
    PyObject_CallMethod(player, "stop", NULL);
    PyGILState_Release(state);

}

void AudioSourcePython::handlePl()
{
    emit plEnabledChanged(false);
}

void AudioSourcePython::handlePrevious()
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

void AudioSourcePython::handlePlay()
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

void AudioSourcePython::handlePause()
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

void AudioSourcePython::handleStop()
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

void AudioSourcePython::handleNext()
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

void AudioSourcePython::handleOpen()
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
    QFuture<void> status = QtConcurrent::run(&AudioSourcePython::doEject, this);
    ejectWatcher.setFuture(status);
}

void AudioSourcePython::handleShuffle()
{
    if(player == nullptr) return;

    this->isShuffleEnabled = !this->isShuffleEnabled;

    auto state = PyGILState_Ensure();
    PyObject_CallMethod(player, "set_shuffle", "i", this->isShuffleEnabled);
    PyGILState_Release(state);

    refreshStatus(false);
}

void AudioSourcePython::handleRepeat()
{
    if(player == nullptr) return;

    this->isRepeatEnabled = !this->isRepeatEnabled;

    auto state = PyGILState_Ensure();
    PyObject_CallMethod(player, "set_repeat", "i", this->isRepeatEnabled);
    PyGILState_Release(state);

    refreshStatus(false);
}

void AudioSourcePython::handleSeek(int mseconds)
{
    if(player == nullptr) return;

    auto state = PyGILState_Ensure();
    PyObject_CallMethod(player, "seek", "l", mseconds);
    PyGILState_Release(state);

    refreshStatus(false);
}

void AudioSourcePython::refreshStatus(bool shouldRefreshTrackInfo)
{
    if(player == nullptr) return;
    auto state = PyGILState_Ensure();

    // Get shuffle status
    PyObject *pyShuffleEnabled = PyObject_CallMethod(player, "get_shuffle", NULL);
    if(pyShuffleEnabled != nullptr && PyBool_Check(pyShuffleEnabled)) {
        this->isShuffleEnabled = PyObject_IsTrue(pyShuffleEnabled);
        emit shuffleEnabledChanged(this->isShuffleEnabled);
    }
    if(pyShuffleEnabled) Py_DECREF(pyShuffleEnabled);

    // Get repeat status
    PyObject *pyRepeatEnabled = PyObject_CallMethod(player, "get_repeat", NULL);
    if(pyRepeatEnabled != nullptr && PyBool_Check(pyRepeatEnabled)) {
        this->isRepeatEnabled = PyObject_IsTrue(pyRepeatEnabled);
        emit repeatEnabledChanged(this->isRepeatEnabled);
    }
    if(pyRepeatEnabled) Py_DECREF(pyRepeatEnabled);

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

    if(status == "idle") {
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

        emit this->messageSet("ERROR", 5000);
    }

    this->currentStatus = status;

    if(status != "idle" && shouldRefreshTrackInfo) {
        refreshTrackInfo();
    }
}

void AudioSourcePython::refreshTrackInfo(bool force)
{
    #ifdef DEBUG_ASPY
    qDebug() << ">>>>>>>>>Refresh track info";
    #endif
    if(player == nullptr) return;
    auto state = PyGILState_Ensure();
    PyObject *pyTrackInfo = PyObject_CallMethod(player, "get_track_info", NULL);
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
    PyObject *pyCodec =  PyTuple_GetItem(pyTrackInfo, 5);
    PyObject *pyBitrate =  PyTuple_GetItem(pyTrackInfo, 6);
    PyObject *pySampleRate =  PyTuple_GetItem(pyTrackInfo, 7);

    quint32 trackNumber = PyLong_AsLong(pyTrackNumber);
    quint32 duration = PyLong_AsLong(pyDuration);
    QString title(PyUnicode_AsUTF8(pyTitle));

    bool isSameTrack =  this->currentMetadata.value(QMediaMetaData::TrackNumber).toInt() == trackNumber &&
                        this->currentMetadata.value(QMediaMetaData::Title).toString() == title &&
                        this->currentMetadata.value(QMediaMetaData::Duration).toInt() == duration;

    if(isSameTrack && !force) {
        // No need to refresh
        Py_DECREF(pyTrackInfo);
        PyGILState_Release(state);
        return;
    }

    QString artist(PyUnicode_AsUTF8(pyArtist));
    QString album(PyUnicode_AsUTF8(pyAlbum));
    QString codec(PyUnicode_AsUTF8(pyCodec));
    quint32 bitrate = PyLong_AsLong(pyBitrate);
    quint32 sampleRate = PyLong_AsLong(pySampleRate);

    QMediaMetaData metadata;
    metadata.insert(QMediaMetaData::Title, title);
    metadata.insert(QMediaMetaData::AlbumArtist, artist);
    metadata.insert(QMediaMetaData::AlbumTitle, album);
    metadata.insert(QMediaMetaData::TrackNumber, trackNumber);
    metadata.insert(QMediaMetaData::Duration, duration);
    metadata.insert(QMediaMetaData::AudioBitRate, bitrate);
    metadata.insert(QMediaMetaData::Comment, QString::number(sampleRate)); // Using Comment as sample rate
    metadata.insert(QMediaMetaData::Description, codec); // Using Description as codec

    this->currentMetadata = metadata;
    emit this->durationChanged(duration);
    emit this->metadataChanged(metadata);

    #ifdef DEBUG_ASPY
    qDebug() << ">>>>>>>>METADATA changed";
    #endif

    Py_DECREF(pyTrackInfo);

    PyGILState_Release(state);

}

void AudioSourcePython::refreshProgress()
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

void AudioSourcePython::interpolateProgress()
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

void AudioSourcePython::refreshMessage()
{
    if(player == nullptr) return;
    auto state = PyGILState_Ensure();

    PyObject *pyMessageData = PyObject_CallMethod(player, "get_message", NULL);
    if(pyMessageData == nullptr) {
        #ifdef DEBUG_ASPY
        qDebug() << ">>> Couldn't get track message data";
        #endif
        PyErr_Print();
        PyGILState_Release(state);
        return;
    }

    // format (show_message: bool, message: str, message_timeout_ms: int)
    PyObject *pyShowMessage = PyTuple_GetItem(pyMessageData, 0);
    PyObject *pyMessage = PyTuple_GetItem(pyMessageData, 1);
    PyObject *pyMessageTimeout = PyTuple_GetItem(pyMessageData, 2);

    quint32 showMessage = PyLong_AsLong(pyShowMessage);
    QString message(PyUnicode_AsUTF8(pyMessage));
    quint32 messageTimeout = PyLong_AsLong(pyMessageTimeout);

    if(showMessage) {
        PyObject_CallMethod(player, "clear_message", NULL);
        emit this->messageSet(message, messageTimeout);
    }

    Py_DECREF(pyMessageData);
    PyGILState_Release(state);
}

void AudioSourcePython::runPythonLoop()
{
    if(player == nullptr) return;
    auto state = PyGILState_Ensure();

    PyObject *pyRunLoop = PyObject_CallMethod(player, "run_loop", NULL);
    if(pyRunLoop == nullptr) {
        #ifdef DEBUG_ASPY
        qDebug() << ">>> Couldn't run Python event loop";
        #endif
        PyErr_Print();
        PyGILState_Release(state);
        return;
    }

    Py_DECREF(pyRunLoop);
    PyGILState_Release(state);
}

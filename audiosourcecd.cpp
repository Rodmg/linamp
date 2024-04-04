#include "audiosourcecd.h"

// Includes for spectrum
#include "spectrumwidget.h"
#include "util.h"
#include "systemaudiocapture_pulseaudio.h"
extern QMutex *sampleMutex;
extern QByteArray *sample;
extern QDataStream *sampleStream;
// END Includes for spectrum

#define ASCD_PROGRESS_INTERPOLATION_TIME 100

AudioSourceCD::AudioSourceCD(QObject *parent)
    : AudioSource{parent}
{
    Py_Initialize();
    // PyEval_InitThreads();
    PyEval_SaveThread();

    auto state = PyGILState_Ensure();

    PyObject *pModuleName = PyUnicode_DecodeFSDefault("cdplayer");
    //PyObject *pModuleName = PyUnicode_DecodeFSDefault("mock_cdplayer");
    cdplayerModule = PyImport_Import(pModuleName);
    Py_DECREF(pModuleName);

    if(cdplayerModule == nullptr) {
        qDebug() << "Couldn't load python module";
        PyGILState_Release(state);
        return;
    }

    PyObject *CDPlayerClass = PyObject_GetAttrString(cdplayerModule, "CDPlayer");

    if(!CDPlayerClass || !PyCallable_Check(CDPlayerClass)) {
        qDebug() << "Error getting CDPlayer Class";
        PyGILState_Release(state);
        return;
    }

    cdplayer = PyObject_CallNoArgs(CDPlayerClass);

    // Timer to detect disc insertion and load
    detectDiscInsertionTimer = new QTimer(this);
    detectDiscInsertionTimer->setInterval(5000);
    connect(detectDiscInsertionTimer, &QTimer::timeout, this, &AudioSourceCD::pollDetectDiscInsertion);
    detectDiscInsertionTimer->start();

    // Watch for async disc detection results
    connect(&pollResultWatcher, &QFutureWatcher<bool>::finished, this, &AudioSourceCD::handlePollResult);

    // Handle load end
    connect(&loadWatcher, &QFutureWatcher<bool>::finished, this, &AudioSourceCD::handleLoadEnd);

    // Handle finish ejecting
    connect(&ejectWatcher, &QFutureWatcher<bool>::finished, this, &AudioSourceCD::handleEjectEnd);


    // Track progress with timer
    progressRefreshTimer = new QTimer(this);
    progressRefreshTimer->setInterval(1000);
    connect(progressRefreshTimer, &QTimer::timeout, this, &AudioSourceCD::refreshProgress);
    progressInterpolateTimer = new QTimer(this);
    progressInterpolateTimer->setInterval(ASCD_PROGRESS_INTERPOLATION_TIME);
    connect(progressInterpolateTimer, &QTimer::timeout, this, &AudioSourceCD::interpolateProgress);

    // Spectrum Analyzer start
    sampleMutex = new QMutex();
    sample = new QByteArray();
    sampleStream = new QDataStream(sample, QIODevice::WriteOnly);
    dataEmitTimer = new QTimer(this);
    dataEmitTimer->setInterval(33); // around 30 fps
    connect(dataEmitTimer, &QTimer::timeout, this, &AudioSourceCD::emitData);

    PyGILState_Release(state);
}

AudioSourceCD::~AudioSourceCD()
{
    //stopPACapture();
    PyGILState_Ensure();
    Py_Finalize();
}

void AudioSourceCD::pollDetectDiscInsertion()
{
    if(pollResultWatcher.isRunning() || loadWatcher.isRunning()) {
        qDebug() << ">>>>>>>>>>>>>>>POLL Avoided";
        return;
    }
    if(pollInProgress) return;
    pollInProgress = true;
    qDebug() << "pollDetectDiscInsertion: polling";
    QFuture<bool> status = QtConcurrent::run(&AudioSourceCD::doPollDetectDiscInsertion, this);
    //pollStatus = &status;
    pollResultWatcher.setFuture(status);
}

void AudioSourceCD::handlePollResult()
{
    qDebug() << ">>>>POLL RESULT";

    bool discDetected = pollResultWatcher.result();
    if(discDetected) {
        if(loadWatcher.isRunning()) {
            qDebug() << ">>>>>>>>>>>>>>>LOAD Avoided";
            return;
        }
        emit this->requestActivation(); // Request audiosource coordinator to select us
        emit this->messageSet("LOADING...", 5000);
        QFuture<void> status = QtConcurrent::run(&AudioSourceCD::doLoad, this);
        loadWatcher.setFuture(status);
    } else {
        pollInProgress = false;
    }
}

bool AudioSourceCD::doPollDetectDiscInsertion()
{
    bool discDetected = false;
    if(cdplayer == nullptr) return discDetected;

    auto state = PyGILState_Ensure();
    PyObject* pyDiscDetected = PyObject_CallMethod(cdplayer, "detect_disc_insertion", NULL);

    if(PyBool_Check(pyDiscDetected)) {
        discDetected = PyObject_IsTrue(pyDiscDetected);

        qDebug() << ">>>Disct detected?:" << discDetected;
    } else {
        qDebug() << ">>>>pollDetectDiscInsertion: Not a bool";
    }
    if(pyDiscDetected) Py_DECREF(pyDiscDetected);
    PyGILState_Release(state);
    return discDetected;
}

void AudioSourceCD::doLoad()
{
    auto state = PyGILState_Ensure();
    PyObject_CallMethod(cdplayer, "load", NULL);
    PyGILState_Release(state);
}

void AudioSourceCD::handleLoadEnd()
{
    emit this->messageClear();
    refreshStatus();
    pollInProgress = false;
}

void AudioSourceCD::doEject()
{
    auto state = PyGILState_Ensure();
    PyObject_CallMethod(cdplayer, "eject", NULL);
    PyGILState_Release(state);
}

void AudioSourceCD::handleEjectEnd()
{
    emit this->messageClear();
    currentTrackNumber = std::numeric_limits<quint32>::max();
    refreshStatus();
}


void AudioSourceCD::activate()
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
}

void AudioSourceCD::deactivate()
{
    progressRefreshTimer->stop();
    stopSpectrum();
    emit playbackStateChanged(MediaPlayer::StoppedState);
    if(cdplayer == nullptr) return;
    auto state = PyGILState_Ensure();
    PyObject_CallMethod(cdplayer, "stop", NULL);
    PyGILState_Release(state);
}

void AudioSourceCD::handlePl()
{
    emit plEnabledChanged(false);
}

void AudioSourceCD::handlePrevious()
{
    if(cdplayer == nullptr) return;
    auto state = PyGILState_Ensure();
    PyObject_CallMethod(cdplayer, "prev", NULL);
    PyGILState_Release(state);
    refreshStatus();
}

void AudioSourceCD::handlePlay()
{
    if(cdplayer == nullptr) return;
    auto state = PyGILState_Ensure();
    PyObject_CallMethod(cdplayer, "play", NULL);
    PyGILState_Release(state);
    refreshStatus();
}

void AudioSourceCD::handlePause()
{
    if(cdplayer == nullptr) return;
    auto state = PyGILState_Ensure();
    PyObject_CallMethod(cdplayer, "pause", NULL);
    PyGILState_Release(state);
    refreshStatus();
}

void AudioSourceCD::handleStop()
{
    if(cdplayer == nullptr) return;
    auto state = PyGILState_Ensure();
    PyObject_CallMethod(cdplayer, "stop", NULL);
    PyGILState_Release(state);
    refreshStatus();
}

void AudioSourceCD::handleNext()
{
    if(cdplayer == nullptr) return;
    auto state = PyGILState_Ensure();
    PyObject_CallMethod(cdplayer, "next", NULL);
    PyGILState_Release(state);
    refreshStatus();
}

void AudioSourceCD::handleOpen()
{
    if(cdplayer == nullptr) return;
    qDebug() << "<<<<<EJECTING";
    if(ejectWatcher.isRunning()) {
        qDebug() << ">>>>>>>>>>>>>>>EJECT Avoided";
        return;
    }
    emit this->messageSet("EJECTING...", 4000);
    QFuture<void> status = QtConcurrent::run(&AudioSourceCD::doEject, this);
    ejectWatcher.setFuture(status);

}

void AudioSourceCD::handleShuffle()
{
    if(cdplayer == nullptr) return;

    this->isShuffleEnabled = !this->isShuffleEnabled;

    auto state = PyGILState_Ensure();
    PyObject_CallMethod(cdplayer, "set_shuffle", "i", this->isShuffleEnabled);
    PyGILState_Release(state);

    refreshStatus(false);
}

void AudioSourceCD::handleRepeat()
{
    if(cdplayer == nullptr) return;

    this->isRepeatEnabled = !this->isRepeatEnabled;

    auto state = PyGILState_Ensure();
    PyObject_CallMethod(cdplayer, "set_repeat", "i", this->isRepeatEnabled);
    PyGILState_Release(state);

    refreshStatus(false);
}

void AudioSourceCD::handleSeek(int mseconds)
{
    if(cdplayer == nullptr) return;

    auto state = PyGILState_Ensure();
    PyObject_CallMethod(cdplayer, "seek", "l", mseconds);
    PyGILState_Release(state);

    refreshStatus(false);
}

void AudioSourceCD::refreshStatus(bool shouldRefreshTrackInfo)
{
    if(cdplayer == nullptr) return;
    auto state = PyGILState_Ensure();

    // Get shuffle status
    PyObject *pyShuffleEnabled = PyObject_CallMethod(cdplayer, "get_shuffle", NULL);
    if(PyBool_Check(pyShuffleEnabled)) {
        this->isShuffleEnabled = PyObject_IsTrue(pyShuffleEnabled);
        emit shuffleEnabledChanged(this->isShuffleEnabled);
    }
    Py_DECREF(pyShuffleEnabled);

    // Get repeat status
    PyObject *pyRepeatEnabled = PyObject_CallMethod(cdplayer, "get_repeat", NULL);
    if(PyBool_Check(pyRepeatEnabled)) {
        this->isRepeatEnabled = PyObject_IsTrue(pyRepeatEnabled);
        emit repeatEnabledChanged(this->isRepeatEnabled);
    }
    Py_DECREF(pyRepeatEnabled);

    PyObject *pyStatus = PyObject_CallMethod(cdplayer, "get_status", NULL);
    if(pyStatus == nullptr) {
        PyGILState_Release(state);
        return;
    }
    QString status(PyUnicode_AsUTF8(pyStatus));
    Py_DECREF(pyStatus);

    PyGILState_Release(state);

    qDebug() << ">>>Status" << status;

    if(status == "no-disc") {
        QMediaMetaData metadata = QMediaMetaData{};
        metadata.insert(QMediaMetaData::Title, "NO DISC");
        emit metadataChanged(metadata);
        emit playbackStateChanged(MediaPlayer::StoppedState);
        emit positionChanged(0);
        emit durationChanged(0);

        progressInterpolateTimer->stop();
        stopSpectrum();
    }

    if(status == "stopped") {
        emit playbackStateChanged(MediaPlayer::StoppedState);
        emit positionChanged(0);

        progressInterpolateTimer->stop();
        stopSpectrum();
    }

    if(status == "playing") {
        emit this->messageClear();
        emit playbackStateChanged(MediaPlayer::PlayingState);

        progressInterpolateTimer->start();
        startSpectrum();
    }

    if(status == "paused") {
        emit this->messageClear();
        emit playbackStateChanged(MediaPlayer::PausedState);

        progressInterpolateTimer->stop();
        stopSpectrum();
    }

    if(status == "loading") {
        emit playbackStateChanged(MediaPlayer::StoppedState);
        emit positionChanged(0);

        progressInterpolateTimer->stop();
        stopSpectrum();

        emit this->messageSet("LOADING...", 3000);
    }

    if(status == "error") {
        emit playbackStateChanged(MediaPlayer::StoppedState);
        emit positionChanged(0);

        progressInterpolateTimer->stop();
        stopSpectrum();

        emit this->messageSet("VLC ERROR", 5000);
    }

    this->currentStatus = status;

    if(status != "no-disc" && shouldRefreshTrackInfo) {
        refreshTrackInfo();
    }
}

void AudioSourceCD::refreshTrackInfo(bool force)
{
    qDebug() << ">>>>>>>>>Refresh track info";
    if(cdplayer == nullptr) return;
    auto state = PyGILState_Ensure();
    PyObject *pyTrackInfo = PyObject_CallMethod(cdplayer, "get_current_track_info", NULL);
    if(pyTrackInfo == nullptr) {
        qDebug() << ">>> Couldn't get track info";
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

    qDebug() << ">>>>>>>>METADATA changed";

    Py_DECREF(pyTrackInfo);

    PyGILState_Release(state);

}

void AudioSourceCD::refreshProgress()
{
    if(cdplayer == nullptr) return;

    refreshStatus();

    auto state = PyGILState_Ensure();

    PyObject *pyPosition = PyObject_CallMethod(cdplayer, "get_postition", NULL);
    if(pyPosition == nullptr) {
        qDebug() << ">>> Couldn't get track position";
        PyErr_Print();
        PyGILState_Release(state);
        return;
    }
    if(PyLong_Check(pyPosition)) {
        quint32 position = PyLong_AsLong(pyPosition);
        this->currentProgress = position;
        emit this->positionChanged(this->currentProgress);
    }
    Py_DECREF(pyPosition);

    PyGILState_Release(state);
}

void AudioSourceCD::interpolateProgress()
{
    this->currentProgress += ASCD_PROGRESS_INTERPOLATION_TIME;
    emit this->positionChanged(this->currentProgress);
}


// Spectrum analyzer functions

void AudioSourceCD::emitData()
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

void AudioSourceCD::startSpectrum()
{
    //QtConcurrent::run(startPACapture);
    //dataEmitTimer->start();
}

void AudioSourceCD::stopSpectrum()
{
    //stopPACapture();
    //dataEmitTimer->stop();
}

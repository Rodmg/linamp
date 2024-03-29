#include "audiosourcecd.h"

AudioSourceCD::AudioSourceCD(QObject *parent)
    : AudioSource{parent}
{
    Py_Initialize();
    PyObject *pModuleName = PyUnicode_DecodeFSDefault("cdplayer");
    cdplayerModule = PyImport_Import(pModuleName);
    Py_DECREF(pModuleName);

    if(cdplayerModule == nullptr) {
        qDebug() << "Couldn't load python module";
        return;
    }

    PyObject *CDPlayerClass = PyObject_GetAttrString(cdplayerModule, "CDPlayer");

    if(!CDPlayerClass || !PyCallable_Check(CDPlayerClass)) {
        qDebug() << "Error getting CDPlayer Class";
        return;
    }

    cdplayer = PyObject_CallNoArgs(CDPlayerClass);

    PyObject_CallMethod(cdplayer, "load", NULL);


    // Timer to detect disc insertion and load
    detectDiscInsertionTimer = new QTimer(this);
    detectDiscInsertionTimer->setInterval(5000);
    connect(detectDiscInsertionTimer, &QTimer::timeout, this, &AudioSourceCD::pollDetectDiscInsertion);
    detectDiscInsertionTimer->start();
}

AudioSourceCD::~AudioSourceCD()
{
    //stopPACapture();
    Py_FinalizeEx();
}

void AudioSourceCD::pollDetectDiscInsertion()
{
    if(cdplayer == nullptr) return;
    PyObject_CallMethod(cdplayer, "detect_disc_insertion", NULL);
}


void AudioSourceCD::activate()
{
    QMediaMetaData metadata = QMediaMetaData{};
    metadata.insert(QMediaMetaData::Title, "CD");

    emit playbackStateChanged(MediaPlayer::StoppedState);
    emit positionChanged(0);
    emit metadataChanged(metadata);
    emit durationChanged(0);
    emit eqEnabledChanged(false);
    emit plEnabledChanged(false);
    emit shuffleEnabledChanged(false);
    emit repeatEnabledChanged(false);

    // TODO

}

void AudioSourceCD::deactivate()
{
    //stopSpectrum();
    emit playbackStateChanged(MediaPlayer::StoppedState);
}

void AudioSourceCD::handlePl()
{

}

void AudioSourceCD::handlePrevious()
{
    if(cdplayer == nullptr) return;
    PyObject_CallMethod(cdplayer, "prev", NULL);
}

void AudioSourceCD::handlePlay()
{
    if(cdplayer == nullptr) return;
    PyObject_CallMethod(cdplayer, "play", NULL);
}

void AudioSourceCD::handlePause()
{
    if(cdplayer == nullptr) return;
    PyObject_CallMethod(cdplayer, "pause", NULL);
}

void AudioSourceCD::handleStop()
{
    if(cdplayer == nullptr) return;
    PyObject_CallMethod(cdplayer, "stop", NULL);
}

void AudioSourceCD::handleNext()
{
    if(cdplayer == nullptr) return;
    PyObject_CallMethod(cdplayer, "next", NULL);
}

void AudioSourceCD::handleOpen()
{
    if(cdplayer == nullptr) return;
    PyObject_CallMethod(cdplayer, "eject", NULL);
}

void AudioSourceCD::handleShuffle()
{

}

void AudioSourceCD::handleRepeat()
{

}

void AudioSourceCD::handleSeek(int mseconds)
{

}

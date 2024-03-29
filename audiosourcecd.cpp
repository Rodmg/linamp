#include "audiosourcecd.h"

AudioSourceCD::AudioSourceCD(QObject *parent)
    : AudioSource{parent}
{
    Py_Initialize();
    PyObject *pModuleName = PyUnicode_DecodeFSDefault("python/cdplayer");
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
    PyObject_CallMethod(cdplayer, "play", NULL);
}

AudioSourceCD::~AudioSourceCD()
{
    //stopPACapture();
    Py_FinalizeEx();
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

}

void AudioSourceCD::handlePlay()
{

}

void AudioSourceCD::handlePause()
{

}

void AudioSourceCD::handleStop()
{

}

void AudioSourceCD::handleNext()
{

}

void AudioSourceCD::handleOpen()
{

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

#include "audiosourcebluetooth.h"

AudioSourceBluetooth::AudioSourceBluetooth(QObject *parent)
    : AudioSource{parent}
{

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
}

void AudioSourceBluetooth::deactivate()
{

}

void AudioSourceBluetooth::handlePl()
{
    emit plEnabledChanged(false);
}

void AudioSourceBluetooth::handlePrevious()
{

}

void AudioSourceBluetooth::handlePlay()
{

}

void AudioSourceBluetooth::handlePause()
{

}

void AudioSourceBluetooth::handleStop()
{

}

void AudioSourceBluetooth::handleNext()
{

}

void AudioSourceBluetooth::handleOpen()
{

}

void AudioSourceBluetooth::handleShuffle()
{
    emit shuffleEnabledChanged(false);
}

void AudioSourceBluetooth::handleRepeat()
{
    emit repeatEnabledChanged(false);
}

void AudioSourceBluetooth::handleSeek(int mseconds)
{

}

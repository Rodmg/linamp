#include "audiosourcecoordinator.h"

AudioSourceCoordinator::AudioSourceCoordinator(QObject *parent, PlayerView *playerView)
    : QObject{parent}
{
    system_audio = new SystemAudioControl(this);
    view = playerView;

    // TODO instantiate sources
    // TODO setSource

    /*
    view->setVolume(system_audio->getVolume());
    connect(system_audio, &SystemAudioControl::volumeChanged, view, &PlayerView::setVolume);
    connect(view, &PlayerView::volumeChanged, this, &AudioSourceCoordinator::setVolume);

    view->setBalance(system_audio->getBalance());
    connect(system_audio, &SystemAudioControl::balanceChanged, view, &PlayerView::setBalance);
    connect(view, &PlayerView::balanceChanged, this, &AudioSourceCoordinator::setBalance);
    */
}

void AudioSourceCoordinator::setSource(int source)
{
    // TODO
    // deactivate old source
    // disconnect slots
    // connect slots to new source
    // activate new source

    this->currentSource = source;
}

void AudioSourceCoordinator::setVolume(int volume)
{
    system_audio->setVolume(volume);
}

void AudioSourceCoordinator::setBalance(int balance)
{
    system_audio->setBalance(balance);
}

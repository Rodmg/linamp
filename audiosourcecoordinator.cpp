#include "audiosourcecoordinator.h"
#include "audiosourcefile.h"

AudioSourceCoordinator::AudioSourceCoordinator(QObject *parent, PlayerView *playerView)
    : QObject{parent}
{
    system_audio = new SystemAudioControl(this);
    view = playerView;

    view->setVolume(system_audio->getVolume());
    connect(system_audio, &SystemAudioControl::volumeChanged, view, &PlayerView::setVolume);
    connect(view, &PlayerView::volumeChanged, this, &AudioSourceCoordinator::setVolume);

    view->setBalance(system_audio->getBalance());
    connect(system_audio, &SystemAudioControl::balanceChanged, view, &PlayerView::setBalance);
    connect(view, &PlayerView::balanceChanged, this, &AudioSourceCoordinator::setBalance);

}

void AudioSourceCoordinator::setSource(int source)
{
    // TODO
    if(currentSource >= 0) {
        // deactivate old source
        sources[currentSource]->deactivate();

        // disconnect slots
    }

    currentSource = source;
    // connect slots to new source
    connect(view, &PlayerView::positionChanged, sources[currentSource], &AudioSource::handleSeek);
    connect(view, &PlayerView::previousClicked, sources[currentSource], &AudioSource::handlePrevious);
    connect(view, &PlayerView::playClicked, sources[currentSource], &AudioSource::handlePlay);
    connect(view, &PlayerView::pauseClicked, sources[currentSource], &AudioSource::handlePause);
    connect(view, &PlayerView::stopClicked, sources[currentSource], &AudioSource::handleStop);
    connect(view, &PlayerView::nextClicked, sources[currentSource], &AudioSource::handleNext);
    connect(view, &PlayerView::openClicked, sources[currentSource], &AudioSource::handleOpen);
    connect(view, &PlayerView::shuffleClicked, sources[currentSource], &AudioSource::handleShuffle);
    connect(view, &PlayerView::repeatClicked, sources[currentSource], &AudioSource::handleRepeat);

    connect(sources[currentSource], &AudioSource::playbackStateChanged, view, &PlayerView::setPlaybackState);
    connect(sources[currentSource], &AudioSource::positionChanged, view, &PlayerView::setPosition);
    connect(sources[currentSource], &AudioSource::dataEmitted, view, &PlayerView::setSpectrumData);
    connect(sources[currentSource], &AudioSource::metadataChanged, view, &PlayerView::setMetadata);
    connect(sources[currentSource], &AudioSource::durationChanged, view, &PlayerView::setDuration);
    connect(sources[currentSource], &AudioSource::eqEnabledChanged, view, &PlayerView::setEqEnabled);
    connect(sources[currentSource], &AudioSource::plEnabledChanged, view, &PlayerView::setPlEnabled);
    connect(sources[currentSource], &AudioSource::shuffleEnabledChanged, view, &PlayerView::setShuffleEnabled);
    connect(sources[currentSource], &AudioSource::repeatEnabledChanged, view, &PlayerView::setRepeatEnabled);
    connect(sources[currentSource], &AudioSource::messageSet, view, &PlayerView::setMessage);
    connect(sources[currentSource], &AudioSource::messageClear, view, &PlayerView::clearMessage);

    // activate new source
    sources[currentSource]->activate();
}

void AudioSourceCoordinator::setVolume(int volume)
{
    system_audio->setVolume(volume);
}

void AudioSourceCoordinator::setBalance(int balance)
{
    system_audio->setBalance(balance);
}

void AudioSourceCoordinator::addSource(AudioSource *source, bool activate)
{
    // Instantiate sources
    sources.append(source);
    if(activate) {
        setSource(sources.length() - 1);
    }
}

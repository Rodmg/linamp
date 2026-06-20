#include "audiosourcecoordinator.h"

AudioSourceCoordinator::AudioSourceCoordinator(QObject *parent, PlayerView *playerView)
    : QObject{parent}
{
    system_audio = new SystemAudioControl(this);
    view = playerView;

    view->setVolume(system_audio->getVolume());
    connect(system_audio, &SystemAudioControl::volumeChanged, view, &PlayerView::setVolume);
    connect(view, &PlayerView::volumeChanged, this, &AudioSourceCoordinator::setVolume);
    connect(view, &PlayerView::volumeDragStarted, this, &AudioSourceCoordinator::onVolumeDragStarted);
    connect(view, &PlayerView::volumeDragFinished, this, &AudioSourceCoordinator::onVolumeDragFinished);

    view->setBalance(system_audio->getBalance());
    connect(system_audio, &SystemAudioControl::balanceChanged, view, &PlayerView::setBalance);
    connect(view, &PlayerView::balanceChanged, this, &AudioSourceCoordinator::setBalance);
    connect(view, &PlayerView::balanceDragStarted, this, &AudioSourceCoordinator::onBalanceDragStarted);
    connect(view, &PlayerView::balanceDragFinished, this, &AudioSourceCoordinator::onBalanceDragFinished);
}

void AudioSourceCoordinator::setSource(int newSource)
{
    if(newSource > sources.length() - 1) {
        qDebug() << "WARNING: Trying to set source out of range";
        return;
    }

    if (currentSource == newSource) {
        qDebug() << "INFO: Source already selected";
        return;
    }

    if(currentSource >= 0) {
        // deactivate old source
        sources[currentSource]->deactivate();
        // disconnect slots
        disconnect(view, &PlayerView::positionChanged, sources[currentSource], &AudioSource::handleSeek);
        disconnect(view, &PlayerView::previousClicked, sources[currentSource], &AudioSource::handlePrevious);
        disconnect(view, &PlayerView::playClicked, sources[currentSource], &AudioSource::handlePlay);
        disconnect(view, &PlayerView::pauseClicked, sources[currentSource], &AudioSource::handlePause);
        disconnect(view, &PlayerView::stopClicked, sources[currentSource], &AudioSource::handleStop);
        disconnect(view, &PlayerView::nextClicked, sources[currentSource], &AudioSource::handleNext);
        disconnect(view, &PlayerView::openClicked, sources[currentSource], &AudioSource::handleOpen);
        disconnect(view, &PlayerView::shuffleClicked, sources[currentSource], &AudioSource::handleShuffle);
        disconnect(view, &PlayerView::repeatClicked, sources[currentSource], &AudioSource::handleRepeat);
        disconnect(view, &PlayerView::plClicked, sources[currentSource], &AudioSource::handlePl);

        disconnect(sources[currentSource], &AudioSource::playbackStateChanged, view, &PlayerView::setPlaybackState);
        disconnect(sources[currentSource], &AudioSource::positionChanged, view, &PlayerView::setPosition);
        disconnect(sources[currentSource], &AudioSource::dataEmitted, view, &PlayerView::setSpectrumData);
        disconnect(sources[currentSource], &AudioSource::metadataChanged, view, &PlayerView::setMetadata);
        disconnect(sources[currentSource], &AudioSource::durationChanged, view, &PlayerView::setDuration);
        disconnect(sources[currentSource], &AudioSource::eqEnabledChanged, view, &PlayerView::setEqEnabled);
        disconnect(sources[currentSource], &AudioSource::plEnabledChanged, view, &PlayerView::setPlEnabled);
        disconnect(sources[currentSource], &AudioSource::shuffleEnabledChanged, view, &PlayerView::setShuffleEnabled);
        disconnect(sources[currentSource], &AudioSource::repeatEnabledChanged, view, &PlayerView::setRepeatEnabled);
        disconnect(sources[currentSource], &AudioSource::messageSet, view, &PlayerView::setMessage);
        disconnect(sources[currentSource], &AudioSource::messageClear, view, &PlayerView::clearMessage);
    }

    currentSource = newSource;
    view->setDeferredSeekEnabled(sourceLabels[currentSource] == "CD");

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
    connect(view, &PlayerView::plClicked, sources[currentSource], &AudioSource::handlePl);

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

    view->setSourceLabel(sourceLabels[currentSource]);

    // activate new source
    sources[currentSource]->activate();
}

void AudioSourceCoordinator::setVolume(int volume)
{
    system_audio->setVolume(volume);
    showVolumeMessage(volume, m_volumeDragging);
}

void AudioSourceCoordinator::setBalance(int balance)
{
    system_audio->setBalance(balance);
    showBalanceMessage(balance, m_balanceDragging);
}

void AudioSourceCoordinator::onVolumeDragStarted()
{
    m_volumeDragging = true;
    showVolumeMessage(system_audio->getVolume(), true);
}

void AudioSourceCoordinator::onVolumeDragFinished(int volume)
{
    m_volumeDragging = false;
    system_audio->setVolume(volume);
    showVolumeMessage(volume, false);
}

void AudioSourceCoordinator::onBalanceDragStarted()
{
    m_balanceDragging = true;
    showBalanceMessage(system_audio->getBalance(), true);
}

void AudioSourceCoordinator::onBalanceDragFinished(int balance)
{
    m_balanceDragging = false;
    system_audio->setBalance(balance);
    showBalanceMessage(balance, false);
}

void AudioSourceCoordinator::showVolumeMessage(int volume, bool persistent)
{
    const QString message = QString("VOLUME: %1%").arg(volume);
    if (persistent) {
        view->setPersistentMessage(message);
    } else {
        view->setMessage(message, 500);
    }
}

void AudioSourceCoordinator::showBalanceMessage(int balance, bool persistent)
{
    QString message;
    if (balance == 0) {
        message = "BALANCE: CENTER";
    } else {
        message = QString("BALANCE: %1% %2")
                      .arg(qAbs(balance))
                      .arg(balance < 0 ? "LEFT" : "RIGHT");
    }

    if (persistent) {
        view->setPersistentMessage(message);
    } else {
        view->setMessage(message, 500);
    }
}

void AudioSourceCoordinator::addSource(AudioSource *source, QString label, bool activate)
{
    // Instantiate sources
    sources.append(source);
    sourceLabels.append(label);
    quint32 idx = sources.length() - 1;
    connect(source, &AudioSource::requestActivation, [=]() {
        this->setSource(idx);
    } );
    if(activate) {
        setSource(idx);
    }
}

#include "systemaudiocontrol.h"
#include <QDebug>

SystemAudioControl::SystemAudioControl(QObject *parent)
    : QObject{parent} {
    init();
    loadAlsaSettings();

    // Start periodically polling for updates on volume settings
    pollTimer = new QTimer(this);
    pollTimer->setInterval(10000);
    connect(pollTimer, &QTimer::timeout, this, QOverload<>::of(&SystemAudioControl::loadAlsaSettings));
    pollTimer->start();
}

SystemAudioControl::~SystemAudioControl() {
    snd_mixer_close(handle);
}

void SystemAudioControl::init() {
    // Open the mixer
    if (snd_mixer_open(&handle, 0) < 0) {
        qDebug() << "Cannot open mixer";
        return;
    }

    // Attach the mixer
    if (snd_mixer_attach(handle, "default") < 0) {
        qDebug() << "Cannot attach mixer";
        snd_mixer_close(handle);
        return;
    }

    // Register the mixer
    if (snd_mixer_selem_register(handle, NULL, NULL) < 0) {
        qDebug() << "Cannot register mixer";
        snd_mixer_close(handle);
        return;
    }

    // Load the mixer
    if (snd_mixer_load(handle) < 0) {
        qDebug() << "Cannot load mixer";
        snd_mixer_close(handle);
        return;
    }

    // Set the element ID
    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, "Master");

    // Get the mixer element
    elem = snd_mixer_find_selem(handle, sid);
    if (!elem) {
        qDebug() << "Cannot find mixer element";
        snd_mixer_close(handle);
        return;
    }

    initSuccess = true;
}

void SystemAudioControl::setVolume(int volume) {
    this->volume = volume;
    applyAlsaSettings();
}

void SystemAudioControl::setBalance(int balance) {
    this->balance = balance;
    applyAlsaSettings();
}

int SystemAudioControl::getVolume() {
    return this->volume;
}

int SystemAudioControl::getBalance() {
    return this->balance;
}

void SystemAudioControl::applyAlsaSettings() {
    if(!initSuccess) return;

    long min, max;

    // Get the volume range
    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);

    long targetVolume = volume * (max - min) / 100 + min;

    // Calculate the left and right volumes based on the balance
    long leftVolume = (balance <= 0) ? targetVolume : targetVolume - (balance * targetVolume) / 100;
    long rightVolume = (balance >= 0) ? targetVolume : targetVolume + (balance * targetVolume) / 100;

    // Set the left and right volumes
    snd_mixer_selem_set_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, leftVolume);
    snd_mixer_selem_set_playback_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT, rightVolume);
}

void SystemAudioControl::loadAlsaSettings() {

    // Need to re-init in order to get fresh values
    // Otherwise we get the same values always
    if(initSuccess) {
        snd_mixer_close(handle);
    }
    init();

    if(!initSuccess) return;

    long min, max, leftVolume, rightVolume;


    // Get the volume range
    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);

    // Get the left and right volumes
    snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &leftVolume);
    snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT, &rightVolume);

    //qDebug() << "min: " << min << "max: " << max;
    //qDebug() << "leftVolume: " << leftVolume << "rightVolume: " << rightVolume;

    // Scale to 0-100 scale
    leftVolume = leftVolume / ((max - min) / 100 + min);
    rightVolume = rightVolume / ((max - min) / 100 + min);

    //qDebug() << "scaled leftVolume: " << leftVolume << "scaled rightVolume: " << rightVolume;

    // Derive target volume
    int oldVolume = volume;
    volume = leftVolume >= rightVolume ? leftVolume : rightVolume;

    // Derive balance
    int oldBalance = balance;
    balance = (rightVolume - leftVolume) * (100.0 / volume);

    if(oldVolume != volume) {
        emit volumeChanged(volume);
    }

    if(oldBalance != balance) {
        emit balanceChanged(balance);
    }

    //qDebug() << "Volume: " << volume << "Balance: " << balance;
}

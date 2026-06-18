#include "systemaudiocontrol.h"

#include <QDebug>
#include <QMetaObject>
#include <QThread>
#include <QtConcurrent>

namespace {
constexpr int APPLY_INTERVAL_MS = 32;
constexpr int POLL_INTERVAL_MS = 10000;
}

SystemAudioControl::SystemAudioControl(QObject *parent)
    : QObject{parent}
{
    {
        QMutexLocker locker(&m_mixerMutex);
        init();
        loadAlsaSettingsLocked();
    }

    m_applyTimer = new QTimer(this);
    m_applyTimer->setInterval(APPLY_INTERVAL_MS);
    m_applyTimer->setSingleShot(true);
    connect(m_applyTimer, &QTimer::timeout, this, &SystemAudioControl::onApplyTimer);

    m_pollTimer = new QTimer(this);
    m_pollTimer->setInterval(POLL_INTERVAL_MS);
    connect(m_pollTimer, &QTimer::timeout, this, &SystemAudioControl::onPollTimer);
    m_pollTimer->start();
}

SystemAudioControl::~SystemAudioControl()
{
    m_shuttingDown = true;

    if (m_applyTimer != nullptr) {
        m_applyTimer->stop();
    }
    if (m_pollTimer != nullptr) {
        m_pollTimer->stop();
    }

    while (m_applyInFlight.load()) {
        QThread::msleep(1);
    }

    QMutexLocker locker(&m_mixerMutex);
    if (initSuccess && handle != nullptr) {
        snd_mixer_close(handle);
        handle = nullptr;
    }
}

void SystemAudioControl::init()
{
    initSuccess = false;
    elem = nullptr;

    if (handle != nullptr) {
        snd_mixer_close(handle);
        handle = nullptr;
    }

    if (snd_mixer_open(&handle, 0) < 0) {
        qDebug() << "Cannot open mixer";
        return;
    }

    if (snd_mixer_attach(handle, "default") < 0) {
        qDebug() << "Cannot attach mixer";
        snd_mixer_close(handle);
        handle = nullptr;
        return;
    }

    if (snd_mixer_selem_register(handle, NULL, NULL) < 0) {
        qDebug() << "Cannot register mixer";
        snd_mixer_close(handle);
        handle = nullptr;
        return;
    }

    if (snd_mixer_load(handle) < 0) {
        qDebug() << "Cannot load mixer";
        snd_mixer_close(handle);
        handle = nullptr;
        return;
    }

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, "Master");

    elem = snd_mixer_find_selem(handle, sid);
    if (!elem) {
        qDebug() << "Cannot find mixer element";
        snd_mixer_close(handle);
        handle = nullptr;
        return;
    }

    initSuccess = true;
}

void SystemAudioControl::setVolume(int newVolume)
{
    volume = newVolume;
    scheduleApply();
}

void SystemAudioControl::setBalance(int newBalance)
{
    balance = newBalance;
    scheduleApply();
}

int SystemAudioControl::getVolume()
{
    return volume;
}

int SystemAudioControl::getBalance()
{
    return balance;
}

void SystemAudioControl::scheduleApply()
{
    if (m_shuttingDown.load()) {
        return;
    }

    m_applyPending.store(true);

    if (!m_applyTimer->isActive()) {
        performApplyAsync();
        m_applyTimer->start(APPLY_INTERVAL_MS);
    }
}

void SystemAudioControl::onApplyTimer()
{
    if (!m_applyPending.load() || m_shuttingDown.load()) {
        return;
    }

    m_applyPending.store(false);
    performApplyAsync();

    if (m_applyPending.load()) {
        m_applyTimer->start(APPLY_INTERVAL_MS);
    }
}

void SystemAudioControl::performApplyAsync()
{
    if (m_shuttingDown.load()) {
        return;
    }

    if (m_applyInFlight.exchange(true)) {
        return;
    }

    const int applyVolume = volume;
    const int applyBalance = balance;

    QtConcurrent::run([this, applyVolume, applyBalance]() {
        {
            QMutexLocker locker(&m_mixerMutex);
            if (!m_shuttingDown.load() && initSuccess) {
                applyAlsaSettingsLocked(applyVolume, applyBalance);
            }
        }

        m_applyInFlight.store(false);

        if (m_applyPending.load() && !m_shuttingDown.load()) {
            QMetaObject::invokeMethod(this, &SystemAudioControl::performApplyAsync, Qt::QueuedConnection);
        }
    });
}

void SystemAudioControl::applyAlsaSettingsLocked(int applyVolume, int applyBalance)
{
    long min = 0;
    long max = 0;

    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);

    const long targetVolume = applyVolume * (max - min) / 100 + min;

    const long leftVolume = (applyBalance <= 0) ? targetVolume : targetVolume - (applyBalance * targetVolume) / 100;
    const long rightVolume = (applyBalance >= 0) ? targetVolume : targetVolume + (applyBalance * targetVolume) / 100;

    snd_mixer_selem_set_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, leftVolume);
    snd_mixer_selem_set_playback_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT, rightVolume);
}

void SystemAudioControl::onPollTimer()
{
    if (m_shuttingDown.load()) {
        return;
    }

    QtConcurrent::run([this]() {
        const int oldVolume = volume;
        const int oldBalance = balance;

        {
            QMutexLocker locker(&m_mixerMutex);
            if (m_shuttingDown.load()) {
                return;
            }

            if (initSuccess) {
                snd_mixer_close(handle);
                handle = nullptr;
            }
            init();

            if (!initSuccess) {
                return;
            }

            loadAlsaSettingsLocked();
        }

        if (m_shuttingDown.load()) {
            return;
        }

        if (volume != oldVolume) {
            emit volumeChanged(volume);
        }
        if (balance != oldBalance) {
            emit balanceChanged(balance);
        }
    });
}

void SystemAudioControl::loadAlsaSettingsLocked()
{
    long min = 0;
    long max = 0;
    long leftVolume = 0;
    long rightVolume = 0;

    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &leftVolume);
    snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT, &rightVolume);

    leftVolume = leftVolume / ((max - min) / 100 + min);
    rightVolume = rightVolume / ((max - min) / 100 + min);

    volume = leftVolume >= rightVolume ? leftVolume : rightVolume;

    if (volume > 0) {
        balance = (rightVolume - leftVolume) * (100.0 / volume);
    } else {
        balance = 0;
    }
}

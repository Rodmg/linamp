#include "systemaudiocontrol.h"
#include "systemequalizer.h"

#include <QThread>
#include <QThreadPool>

#include <algorithm>

namespace {
constexpr int APPLY_INTERVAL_MS = 32;

int clampUi(int value, int min, int max)
{
    return std::max(min, std::min(max, value));
}
}

SystemAudioControl::SystemAudioControl(QObject *parent)
    : QObject{parent}
{
    loadSettings();

    m_applyTimer = new QTimer(this);
    m_applyTimer->setInterval(APPLY_INTERVAL_MS);
    m_applyTimer->setSingleShot(true);
    connect(m_applyTimer, &QTimer::timeout, this, &SystemAudioControl::onApplyTimer);
}

SystemAudioControl::~SystemAudioControl()
{
    m_shuttingDown = true;
    if (m_applyTimer != nullptr) {
        m_applyTimer->stop();
    }
    while (m_applyInFlight.load()) {
        QThread::msleep(1);
    }
    m_settings.sync();
}

void SystemAudioControl::setEqualizer(SystemEqualizer *equalizer)
{
    m_equalizer = equalizer;
    applyToHardware();
    // The card route comes up centered and overwrites channel balance after
    // the first write. Apply again once that restore has finished.
    const int delaysMs[] = {250, 1000, 2500};
    for (int delayMs : delaysMs) {
        QTimer::singleShot(delayMs, this, [this] { applyToHardware(); });
    }
}

void SystemAudioControl::setVolume(int newVolume)
{
    newVolume = clampUi(newVolume, 0, 100);
    if (volume == newVolume) {
        return;
    }
    volume = newVolume;
    writeSettings();
    scheduleApply();
}

void SystemAudioControl::setBalance(int newBalance)
{
    newBalance = clampUi(newBalance, -100, 100);
    if (balance == newBalance) {
        return;
    }
    balance = newBalance;
    writeSettings();
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

void SystemAudioControl::reapply()
{
    applyToHardware();
}

void SystemAudioControl::loadSettings()
{
    m_settings.beginGroup("audio");
    volume = clampUi(m_settings.value("volume", 100).toInt(), 0, 100);
    balance = clampUi(m_settings.value("balance", 0).toInt(), -100, 100);
    m_settings.endGroup();
}

void SystemAudioControl::writeSettings()
{
    m_settings.beginGroup("audio");
    m_settings.setValue("volume", volume);
    m_settings.setValue("balance", balance);
    m_settings.endGroup();
}

void SystemAudioControl::scheduleApply()
{
    if (m_shuttingDown.load()) {
        return;
    }
    // Pending means the slider moved after the value captured for the
    // in-flight write. Only a real move sets it, so the loop stops when
    // the user stops dragging.
    m_applyPending.store(true);
    if (m_applyTimer->isActive()) {
        return;
    }
    if (m_applyInFlight.load()) {
        m_applyTimer->start(APPLY_INTERVAL_MS);
        return;
    }
    m_applyPending.store(false);
    performApplyAsync();
    m_applyTimer->start(APPLY_INTERVAL_MS);
}

void SystemAudioControl::onApplyTimer()
{
    if (m_shuttingDown.load()) {
        return;
    }
    if (!m_applyPending.load()) {
        m_settings.sync();
        return;
    }
    if (m_applyInFlight.load()) {
        m_applyTimer->start(APPLY_INTERVAL_MS);
        return;
    }
    m_applyPending.store(false);
    performApplyAsync();
    m_applyTimer->start(APPLY_INTERVAL_MS);
}

void SystemAudioControl::performApplyAsync()
{
    if (m_shuttingDown.load()) {
        return;
    }
    if (m_applyInFlight.exchange(true)) {
        return;
    }
    m_applyPending.store(false);

    const int applyVolume = volume;
    const int applyBalance = balance;

    QThreadPool::globalInstance()->start([this, applyVolume, applyBalance]() {
        if (!m_shuttingDown.load() && m_equalizer != nullptr) {
            m_equalizer->setHardwareVolume(applyVolume, applyBalance);
        }
        m_applyInFlight.store(false);
    });
}

void SystemAudioControl::applyToHardware()
{
    scheduleApply();
}

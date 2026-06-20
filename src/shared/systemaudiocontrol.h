#ifndef SYSTEMAUDIOCONTROL_H
#define SYSTEMAUDIOCONTROL_H

#include <QObject>
#include <QMutex>
#include <QTimer>
#include <atomic>

#include <alsa/asoundlib.h>

class SystemAudioControl: public QObject {
    Q_OBJECT
public:
    explicit SystemAudioControl(QObject *parent = nullptr);
    ~SystemAudioControl();

    void setVolume(int volume);
    void setBalance(int balance);

    int getVolume();
    int getBalance();

signals:
    void volumeChanged(int volume);
    void balanceChanged(int balance);

private slots:
    void onApplyTimer();
    void onPollTimer();

private:
    snd_mixer_t *handle = nullptr;
    snd_mixer_selem_id_t *sid = nullptr;
    snd_mixer_elem_t *elem = nullptr;

    QTimer *m_applyTimer = nullptr;
    QTimer *m_pollTimer = nullptr;
    QMutex m_mixerMutex;

    std::atomic_bool m_shuttingDown{false};
    std::atomic_bool m_applyInFlight{false};
    std::atomic_bool m_applyPending{false};

    bool initSuccess = false;

    int volume = 100; // 0 to 100
    int balance = 0; // -100 to 100

    void init();
    void scheduleApply();
    void performApplyAsync();
    void applyAlsaSettingsLocked(int applyVolume, int applyBalance);
    void loadAlsaSettingsLocked();
};

#endif // SYSTEMAUDIOCONTROL_H

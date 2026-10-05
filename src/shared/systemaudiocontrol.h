#ifndef SYSTEMAUDIOCONTROL_H
#define SYSTEMAUDIOCONTROL_H

#include <QObject>
#include <QSettings>
#include <QTimer>
#include <atomic>

class SystemEqualizer;

class SystemAudioControl: public QObject {
    Q_OBJECT
public:
    explicit SystemAudioControl(QObject *parent = nullptr);
    ~SystemAudioControl();

    void setEqualizer(SystemEqualizer *equalizer);
    void setVolume(int volume);
    void setBalance(int balance);
    void reapply();

    int getVolume();
    int getBalance();

signals:
    void volumeChanged(int volume);
    void balanceChanged(int balance);

private slots:
    void onApplyTimer();
    void performApplyAsync();

private:
    SystemEqualizer *m_equalizer = nullptr;
    QSettings m_settings;
    QTimer *m_applyTimer = nullptr;
    std::atomic_bool m_shuttingDown{false};
    std::atomic_bool m_applyInFlight{false};
    std::atomic_bool m_applyPending{false};

    int volume = 100; // 0 to 100
    int balance = 0; // -100 to 100

    void loadSettings();
    void writeSettings();
    void scheduleApply();
    void applyToHardware();
};

#endif // SYSTEMAUDIOCONTROL_H

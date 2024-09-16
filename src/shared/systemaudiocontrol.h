#ifndef SYSTEMAUDIOCONTROL_H
#define SYSTEMAUDIOCONTROL_H

#include <QObject>
#include <QTimer>
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

private:
    snd_mixer_t *handle;
    snd_mixer_selem_id_t *sid;
    snd_mixer_elem_t *elem;

    QTimer *pollTimer = nullptr;

    bool initSuccess = false;

    int volume = 100; // 0 to 100
    int balance = 0; // -100 to 100

    void init();
    void applyAlsaSettings();
    void loadAlsaSettings();

};

#endif // SYSTEMAUDIOCONTROL_H

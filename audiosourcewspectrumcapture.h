#ifndef AUDIOSOURCEWSPECTRUMCAPTURE_H
#define AUDIOSOURCEWSPECTRUMCAPTURE_H

#include <QObject>
#include <QTimer>
#include <QtConcurrent>
#include <signal.h>
#include <spa/param/audio/format-utils.h>
#include <pipewire/pipewire.h>

#include "audiosource.h"

struct PwData {
        struct pw_main_loop *loop;
        struct pw_stream *stream;

        struct spa_audio_info format;
        unsigned move:1;

        QMutex *sampleMutex;
        QByteArray *sample;
        QDataStream *sampleStream;
};

class AudioSourceWSpectrumCapture : public AudioSource
{
    Q_OBJECT
public:
    explicit AudioSourceWSpectrumCapture(QObject *parent = nullptr);
    ~AudioSourceWSpectrumCapture();

    void startSpectrum();
    void stopSpectrum();

private:
    bool spectrumRunning = false;
    QTimer *dataEmitTimer = nullptr;
    void emitData();

    QAudioFormat spectrumDataFormat;

    struct PwData pwData = { 0, };
    void pwLoop();

    QFuture<void> pwLoopThread;

signals:

};

#endif // AUDIOSOURCEWSPECTRUMCAPTURE_H

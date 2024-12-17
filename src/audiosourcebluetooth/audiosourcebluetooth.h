#ifndef AUDIOSOURCEBLUETOOTH_H
#define AUDIOSOURCEBLUETOOTH_H

#define PY_SSIZE_T_CLEAN
#undef slots
#include <Python.h>
#define slots Q_SLOTS

#include <QObject>
#include <QTimer>
#include <QtConcurrent>

#include "audiosourcewspectrumcapture.h"

class AudioSourceBluetooth : public AudioSourceWSpectrumCapture
{
    Q_OBJECT
public:
    explicit AudioSourceBluetooth(QObject *parent = nullptr);
    ~AudioSourceBluetooth();

public slots:
    void activate();
    void deactivate();
    void handlePl();
    void handlePrevious();
    void handlePlay();
    void handlePause();
    void handleStop();
    void handleNext();
    void handleOpen();
    void handleShuffle();
    void handleRepeat();
    void handleSeek(int mseconds);

private:
    bool isActive = false;

    PyObject *playerModule;
    PyObject *player;

    QTimer *progressRefreshTimer = nullptr;
    QTimer *progressInterpolateTimer = nullptr;
    QElapsedTimer progressInterpolateElapsedTimer;
    quint32 currentProgress = 0;
    void refreshProgress();
    void interpolateProgress();

    // Poll events thread
    QTimer *pollEventsTimer = nullptr;
    bool pollInProgress = false;
    void pollEvents();
    bool doPollEvents();
    void handlePollResult();
    QFutureWatcher<bool> pollResultWatcher;

    // Load details thread
    void doLoad();
    void handleLoadEnd();
    QFutureWatcher<void> loadWatcher;

    // Eject thread
    void doEject();
    void handleEjectEnd();
    QFutureWatcher<void> ejectWatcher;

    QString currentStatus; // Status as it comes from python
    bool isShuffleEnabled = false;
    bool isRepeatEnabled = false;

    void refreshStatus(bool shouldRefreshTrackInfo = true);
    void refreshTrackInfo(bool force = false);

    quint32 currentTrackNumber = std::numeric_limits<quint32>::max();
};

#endif // AUDIOSOURCEBLUETOOTH_H

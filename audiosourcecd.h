#ifndef AUDIOSOURCECD_H
#define AUDIOSOURCECD_H

#define PY_SSIZE_T_CLEAN
#undef slots
#include <Python.h>
#define slots Q_SLOTS

#include <QObject>
#include <QTimer>

#include "audiosource.h"

class AudioSourceCD : public AudioSource
{
    Q_OBJECT
public:
    explicit AudioSourceCD(QObject *parent = nullptr);
    ~AudioSourceCD();

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

    PyObject *cdplayerModule;
    PyObject *cdplayer;

    QTimer *detectDiscInsertionTimer = nullptr;
    bool pollInProgress = false;
    void pollDetectDiscInsertion();

    void refreshStatus();
    void refreshTrackInfo();

};

#endif // AUDIOSOURCECD_H

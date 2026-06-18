#ifndef CDNATIVEPLAYBACKENGINE_H
#define CDNATIVEPLAYBACKENGINE_H

#include <QAudioSink>
#include <QFuture>
#include <QList>
#include <QObject>
#include <QString>
#include <QTimer>
#include <atomic>

#include "cdnativetrack.h"

class CDPcmRingBuffer;
class CDPcmIODevice;

class CDNativePlaybackEngine : public QObject
{
    Q_OBJECT
public:
    explicit CDNativePlaybackEngine(QObject *parent = nullptr);
    ~CDNativePlaybackEngine() override;

    void setTracks(const QList<CDNativeTrack> &tracks);
    QList<CDNativeTrack> tracks() const;
    void setDevicePath(const QString &devicePath);

    void setShuffleEnabled(bool enabled);
    void setRepeatAllEnabled(bool enabled);

    bool shuffleEnabled() const;
    bool repeatAllEnabled() const;

    void play();
    void pause();
    void stop();
    void prepareForEject();

    void next();
    void previous();
    void seek(qint64 mseconds);

    qint64 position() const;
    qint64 duration() const;
    int currentTrackIndex() const;

signals:
    void positionChanged(qint64 positionMs);
    void durationChanged(qint64 durationMs);
    void activeTrackChanged(int index);
    void bufferingChanged(bool buffering);
    void ejectPreparationFinished();
    void playbackFinished();

private:
    QList<CDNativeTrack> m_tracks;
    QList<int> m_playOrder;
    int m_currentLogicalIndex = 0;
    int m_currentOrderIndex = 0;
    qint64 m_positionMs = 0;
    qint64 m_durationMs = 0;
    QString m_devicePath = "/dev/cdrom";

    std::atomic_bool m_shuffleEnabled{false};
    std::atomic_bool m_repeatAllEnabled{false};

    QAudioFormat m_audioFormat;
    CDPcmRingBuffer *m_ringBuffer = nullptr;
    CDPcmIODevice *m_pcmDevice = nullptr;
    QAudioSink *m_audioSink = nullptr;
    qint64 m_positionAnchorMs = 0;
    bool m_canResumeFromPause = false;
    bool m_transportStartPending = false;
    bool m_ejectPreparationPending = false;
    bool m_transitionMutePending = false;

    QTimer m_positionTimer;

    std::atomic_bool m_readerRunning{false};
    std::atomic_bool m_readerStopRequested{false};
    std::atomic_uint64_t m_streamGeneration{1};
    QFuture<void> m_readerFuture;

    void rebuildPlayOrder();
    int findOrderIndexForTrack(int trackIndex) const;
    int resolveTrackIndexForPosition(qint64 positionMs) const;
    qint64 trackStartMs(int index) const;
    qint64 trackPlaybackDurationMs(int index) const;
    qint64 trackLastLba(int index) const;

    void updatePositionTick();
    void finishNaturalPlayback();
    void advanceToNextShuffleTrack();
    void ensureSink();
    void recreateSink();
    void resetSinkStream();
    void startTransportWhenReady();
    void finishEjectPreparationWhenReady();
    qint64 positionMsToAbsoluteLba(qint64 positionMs) const;
    qint64 discFirstLba() const;
    qint64 discLastLba() const;

    void startReaderLoop();
    void stopReaderLoop(bool waitForFinish = true);
    void readerLoop();
};

#endif // CDNATIVEPLAYBACKENGINE_H

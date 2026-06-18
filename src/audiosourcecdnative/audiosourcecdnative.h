#ifndef AUDIOSOURCECDNATIVE_H
#define AUDIOSOURCECDNATIVE_H

#include <QFutureWatcher>
#include <QTimer>

#include "audiosourcewspectrumcapture.h"
#include "cdnativediscservice.h"
#include "cdnativemetadataservice.h"

class CDNativePlaybackEngine;

class AudioSourceCDNative : public AudioSourceWSpectrumCapture
{
    Q_OBJECT
public:
    explicit AudioSourceCDNative(QObject *parent = nullptr);
    ~AudioSourceCDNative() override;

public slots:
    void activate() override;
    void deactivate() override;
    void handlePl() override;
    void handlePrevious() override;
    void handlePlay() override;
    void handlePause() override;
    void handleStop() override;
    void handleNext() override;
    void handleOpen() override;
    void handleShuffle() override;
    void handleRepeat() override;
    void handleSeek(int mseconds) override;

private:
    bool m_isActive = false;
    bool m_discLoaded = false;
    bool m_ejectInProgress = false;

    bool m_shuffleEnabled = false;
    bool m_repeatEnabled = false;

    CDNativeDiscService m_discService;
    CDNativeMetadataService m_metadataService;
    CDNativePlaybackEngine *m_engine = nullptr;
    QList<CDNativeTrack> m_tracks;

    QTimer m_detectDiscTimer;
    QFutureWatcher<bool> m_ejectWatcher;
    int m_lastTrackIndex = -1;

    void pollDisc();
    void loadDisc();
    void unloadDisc();

    int resolveTrackIndexForAbsolutePosition(qint64 absoluteMs) const;
    qint64 trackStartMs(int index) const;
    qint64 trackPlaybackDurationMs(int index) const;

    void emitNoDiscMetadata();
    void emitTrackMetadata(int index);
    void finalizeEject();
    void completeEject();
};

#endif // AUDIOSOURCECDNATIVE_H

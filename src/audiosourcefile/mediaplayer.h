// Based on https://stackoverflow.com/questions/41197576/how-to-play-mp3-file-using-qaudiooutput-and-qaudiodecoder
#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include "qmediametadata.h"
#include "qurl.h"
#include <QAudioFormat>
#include <QObject>

class MediaPlayerBackend;

class MediaPlayer : public QObject
{
    Q_OBJECT

public:
    MediaPlayer(QObject *parent = nullptr);

    enum PlaybackState {
        StoppedState,
        PlayingState,
        PausedState
    };
    enum MediaStatus {
        NoMedia,
        LoadingMedia,
        LoadedMedia,
        StalledMedia,
        BufferingMedia,
        BufferedMedia,
        EndOfMedia,
        InvalidMedia
    };
    enum Error {
        NoError,
        ResourceError,
        FormatError,
        NetworkError,
        AccessDeniedError
    };

    void play();
    void pause();
    void stop(bool stopAudioOutput = true);

    PlaybackState playbackState() const;

    qint64 duration() const;
    qint64 position() const;
    float bufferProgress() const;
    MediaStatus mediaStatus() const;
    float volume() const;
    QMediaMetaData metaData() const;
    Error error() const;
    QString errorString() const;
    QAudioFormat format();

    ~MediaPlayer() override;

private:
    MediaPlayerBackend *m_backend = nullptr;

    QAudioFormat m_format;
    QMediaMetaData m_metaData = QMediaMetaData{};
    Error m_error = NoError;
    MediaStatus m_status = MediaStatus::NoMedia;
    PlaybackState m_state = PlaybackState::StoppedState;

    qint64 m_position = 0;
    qint64 m_duration = 0;
    float m_bufferProgress = 0.0;
    float m_volume = 1.0; // range: 0.0 - 1.0
    QString m_errorString;
    bool m_hasSource = false;
    QUrl m_sourceUrl;

    static bool isMissingTitle(const QMediaMetaData &metaData);
    static bool isMissingAlbumTitle(const QMediaMetaData &metaData);
    static bool isMissingAlbumArtist(const QMediaMetaData &metaData);
    static bool isMissingGenre(const QMediaMetaData &metaData);
    static bool isMissingTrackNumber(const QMediaMetaData &metaData);
    static bool isMissingDate(const QMediaMetaData &metaData);
    static bool isMissingBitrate(const QMediaMetaData &metaData);
    static bool isMissingSampleRate(const QMediaMetaData &metaData);
    static bool isMissingDuration(const QMediaMetaData &metaData);

    static PlaybackState mapPlaybackState(int state);
    static MediaStatus mapMediaStatus(int status);
    static Error mapError(int error);

private slots:
    void handleBackendPlaybackStateChanged(int state);
    void handleBackendMediaStatusChanged(int status);
    void handleBackendDurationChanged(qint64 duration);
    void handleBackendPositionChanged(qint64 position);
    void handleBackendBufferProgressChanged(float progress);
    void handleBackendVolumeChanged(float volume);
    void handleBackendMetaDataChanged(const QMediaMetaData &metaData);
    void handleBackendErrorChanged(int error, const QString &errorString);

public slots:
    void setSource(const QUrl &source);
    void clearSource();
    void setPosition(qint64 position);
    void setVolume(float volume);

signals:
    void playbackStateChanged(MediaPlayer::PlaybackState state);
    void mediaStatusChanged(MediaPlayer::MediaStatus status);
    void durationChanged(qint64 duration);
    void positionChanged(qint64 position);
    void bufferProgressChanged(float progress);
    void volumeChanged(float volume);
    void metaDataChanged();
    void errorChanged();
};

#endif // MEDIAPLAYER_H

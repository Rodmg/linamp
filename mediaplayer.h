// Based on https://stackoverflow.com/questions/41197576/how-to-play-mp3-file-using-qaudiooutput-and-qaudiodecoder
#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include "qmediametadata.h"
#include "qurl.h"
#include <QIODevice>
#include <QBuffer>
#include <QAudioDecoder>
#include <QAudioFormat>
#include <QFile>
#include <QAudioSink>
#include <QMutex>

// Class for decode audio files like MP3 and push decoded audio data to QOutputDevice (like speaker) and also signal newData().
// For decoding it uses QAudioDecoder which uses QAudioFormat for decode audio file for desire format, then put decoded data to buffer.
// based on: https://github.com/Znurre/QtMixer
class MediaPlayer : public QIODevice
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

protected:
    qint64 readData(char* data, qint64 maxlen) override;
    qint64 writeData(const char* data, qint64 len) override;

private:
    QBuffer m_input;
    QBuffer m_output;
    QByteArray m_data;
    QAudioFormat m_format;
    QAudioDecoder *m_decoder = nullptr;
    QAudioSink *m_audioOutput = nullptr;
    QUrl m_source;
    QMediaMetaData m_metaData = QMediaMetaData{};
    QMutex initMutex;
    QMutex readMutex;

    Error m_error = NoError;
    MediaStatus m_status = MediaStatus::NoMedia;
    PlaybackState m_state = PlaybackState::StoppedState;

    bool isInited;
    bool isDecodingFinished;

    bool m_seekable = false;
    qint64 m_position = 0;
    float m_volume = 1.0; // range: 0.0 - 1.0

    bool init(const QAudioFormat& format);
    void setupDecoder();
    void setupAudioOutput();
    void clearDecoder();
    void clearAudioOutput();
    void clear();
    bool atEnd() const override;
    void loadMetaData();
    void setError(Error error);
    void setMediaStatus(MediaStatus status);

public slots:
    void setSource(const QUrl &source);
    void clearSource();
    void setPosition(qint64 position);
    void setVolume(float volume);

private slots:
    void bufferReady();
    void finished();
    void onPositionChanged();
    void onDurationChanged(qint64 duration);
    void onDecoderError(QAudioDecoder::Error error);
    void onAtEnd();

signals:
    void playbackStateChanged(MediaPlayer::PlaybackState state);
    void mediaStatusChanged(MediaPlayer::MediaStatus status);
    void newData(const QByteArray& data);
    void durationChanged(qint64 duration);
    void positionChanged(qint64 position);
    void bufferProgressChanged(float progress);
    void volumeChanged(float volume);
    void metaDataChanged();
    void errorChanged();
};

#endif // MEDIAPLAYER_H

// Based on https://stackoverflow.com/questions/41197576/how-to-play-mp3-file-using-qaudiooutput-and-qaudiodecoder
#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include <QIODevice>
#include <QBuffer>
#include <QAudioDecoder>
#include <QAudioFormat>
#include <QFile>
#include <QAudioSink>

// Class for decode audio files like MP3 and push decoded audio data to QOutputDevice (like speaker) and also signal newData().
// For decoding it uses QAudioDecoder which uses QAudioFormat for decode audio file for desire format, then put decoded data to buffer.
// based on: https://github.com/Znurre/QtMixer
class MediaPlayer : public QIODevice
{
    Q_OBJECT

public:
    MediaPlayer(QObject *parent = nullptr);
    bool init(const QAudioFormat& format);

    enum PlaybackState { StoppedState, PlayingState, PausedState };
    enum MediaStatus { NoMedia, LoadingMedia, LoadedMedia, StalledMedia, BufferingMedia, BufferedMedia, EndOfMedia, InvalidMedia };

    void play();
    void pause();
    void stop();

    bool atEnd() const override;

    PlaybackState playbackState() const;

    qint64 duration() const;
    qint64 position() const;

protected:
    qint64 readData(char* data, qint64 maxlen) override;
    qint64 writeData(const char* data, qint64 len) override;

private:
    QBuffer m_input;
    QBuffer m_output;
    QByteArray m_data;
    QAudioDecoder m_decoder;
    QAudioFormat m_format;
    QAudioSink *m_audioOutput;

    PlaybackState m_state;

    bool isInited;
    bool isDecodingFinished;

    void clear();

public slots:
    void setSource(const QUrl &source);
    void setPosition(qint64 position);

private slots:
    void bufferReady();
    void finished();

signals:
    void playbackStateChanged(MediaPlayer::PlaybackState state);
    void newData(const QByteArray& data);
    void durationChanged(qint64 duration);
    void positionChanged(qint64 position);
    float bufferProgress() const;
};

#endif // MEDIAPLAYER_H

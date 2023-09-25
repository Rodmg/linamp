#include "mediaplayer.h"
#include "qurl.h"
#include <QDebug>
#include <QMediaDevices>

MediaPlayer::MediaPlayer(QObject *parent) :
    m_input(&m_data),
    m_output(&m_data),
    m_state(PlaybackState::StoppedState)
{
    setOpenMode(QIODevice::ReadOnly);

    isInited = false;
    isDecodingFinished = false;
}

// format - it is audio format to which we whant decode audio data
bool MediaPlayer::init(const QAudioFormat& format)
{
    m_format = format;
    //m_decoder.setAudioFormat(m_format);

    connect(&m_decoder, SIGNAL(bufferReady()), this, SLOT(bufferReady()));
    connect(&m_decoder, SIGNAL(finished()), this, SLOT(finished()));
    connect(&m_decoder, &QAudioDecoder::durationChanged, this, &MediaPlayer::durationChanged);
    connect(&m_decoder, &QAudioDecoder::positionChanged, this, &MediaPlayer::positionChanged);

    // Initialize buffers
    if (!m_output.open(QIODevice::ReadOnly) || !m_input.open(QIODevice::WriteOnly))
    {
        return false;
    }

    m_audioOutput = new QAudioSink(format, this);
    //connect(m_audioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)));
    m_audioOutput->start(this);

    isInited = true;

    return true;
}

// AudioOutput device (like speaker) call this function for get new audio data
qint64 MediaPlayer::readData(char* data, qint64 maxlen)
{
    memset(data, 0, maxlen);

    if (m_state == PlaybackState::PlayingState)
    {
        m_output.read(data, maxlen);

        // There is we send readed audio data via signal, for ability get audio signal for the who listen this signal.
        // Other word this emulate QAudioProbe behaviour for retrieve audio data which of sent to output device (speaker).
        if (maxlen > 0)
        {
            QByteArray buff(data, maxlen);
            emit newData(buff);
        }

        // Is finish of file
        if (atEnd())
        {
            stop();
        }
    }

    return maxlen;
}

qint64 MediaPlayer::writeData(const char* data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}

// Start play audio file
void MediaPlayer::play()
{
    clear();

    m_decoder.start();

    m_state = PlaybackState::PlayingState;
    emit playbackStateChanged(m_state);
}

void MediaPlayer::pause()
{
    m_decoder.stop();

    m_state = PlaybackState::PausedState;
    emit playbackStateChanged(m_state);
}

// Stop play audio file
void MediaPlayer::stop()
{
    clear();
    m_state = PlaybackState::StoppedState;
    emit playbackStateChanged(m_state);
}


void MediaPlayer::clear()
{
    m_decoder.stop();
    m_data.clear();
    isDecodingFinished = false;
}

// Is finish of file
bool MediaPlayer::atEnd() const
{
    return m_output.size()
           && m_output.atEnd()
           && isDecodingFinished;
}



/////////////////////////////////////////////////////////////////////
// QAudioDecoder logic this methods responsible for decode audio file and put audio data to stream buffer

// Run when decode decoded some audio data
void MediaPlayer::bufferReady() // SLOT
{
    const QAudioBuffer &buffer = m_decoder.read();

    const int length = buffer.byteCount();
    const char *data = buffer.constData<char>();

    m_input.write(data, length);
}

// Run when decode finished decoding
void MediaPlayer::finished() // SLOT
{
    isDecodingFinished = true;
}


/////////////////////////////////////////////////////////////////////

MediaPlayer::PlaybackState MediaPlayer::playbackState() const
{
    // In case if EndOfMedia status is already received
    // but state is not.
    /*if (control
        && control->mediaStatus() == MediaPlayer::EndOfMedia
        && state != control->state()) {
        return control->state();
    }*/

    return m_state;
}

qint64 MediaPlayer::duration() const
{
    return m_decoder.duration();
}

qint64 MediaPlayer::position() const
{
    return m_decoder.position();
}

/*float MediaPlayer::bufferProgress() const
{
    // TODO
    return 100.0;
}*/

void MediaPlayer::setSource(const QUrl &source)
{
    QAudioDevice info(QMediaDevices::defaultAudioOutput());
    qInfo() << source.toString();
    m_decoder.setSource(source);
    QAudioFormat format = info.preferredFormat();
    m_decoder.setAudioFormat(format);
    init(format);
}

void MediaPlayer::setPosition(qint64 position) {
    const qint64 target = position
                       * (m_format.sampleFormat())
                       * (m_format.sampleRate() / 1000)
                       * (m_format.channelCount());

    m_output.seek(target);
}

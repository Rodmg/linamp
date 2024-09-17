#include "mediaplayer.h"
#include "qurl.h"
#include "util.h"
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>
#include <QDebug>
#include <QMediaDevices>
#include <QCoreApplication>
#include <QTimer>

/*
 * Max number of times the player will retry playing
 * after a buffer underrun while loading the file
 * before stopping
 */
#define MP_MAX_BUFFER_UNDERRUN_RETRIES 10

MediaPlayer::MediaPlayer(QObject *parent) :
    QIODevice{parent},
    m_input(&m_data),
    m_output(&m_data),
    m_state(PlaybackState::StoppedState)
{
    setOpenMode(QIODevice::ReadOnly);

    isInited = false;
    isDecodingFinished = false;
}

// format - the format to which we will decode the data for output (PCM)
bool MediaPlayer::init(const QAudioFormat& format)
{
    QMutexLocker l(&initMutex);
    m_format = format;

    // Initialize buffers
    if (!m_output.open(QIODevice::ReadOnly) || !m_input.open(QIODevice::WriteOnly))
    {
        qDebug() << "Error initing buffers";
        return false;
    }

    setupDecoder();
    setupAudioOutput();

    isInited = true;

    return true;
}

void MediaPlayer::setupDecoder()
{
    if(!m_decoder) {
        m_decoder = new QAudioDecoder(this);
    }
    m_decoder->setAudioFormat(m_format);
    connect(m_decoder, &QAudioDecoder::bufferReady, this, &MediaPlayer::bufferReady);
    connect(m_decoder, &QAudioDecoder::finished, this, &MediaPlayer::finished);
    connect(m_decoder, &QAudioDecoder::durationChanged, this, &MediaPlayer::onDurationChanged);
    connect(m_decoder, QOverload<QAudioDecoder::Error>::of(&QAudioDecoder::error), this, &MediaPlayer::onDecoderError);
    connect(this, &MediaPlayer::newData, this, &MediaPlayer::onPositionChanged);
}

void MediaPlayer::setupAudioOutput()
{
    if(m_audioOutput) {
        delete m_audioOutput;
    }
    m_audioOutput = new QAudioSink(m_format, this);
    connect(m_audioOutput, &QAudioSink::stateChanged, this, &MediaPlayer::onOutputStateChanged);

    m_audioOutput->setVolume(m_volume);
    emit volumeChanged(volume());
}

void MediaPlayer::clearDecoder()
{
    if(!m_decoder) return;
    m_decoder->stop();
    m_decoder->disconnect();
}

void MediaPlayer::clearAudioOutput()
{
    if (!m_audioOutput) return;
    m_audioOutput->stop();
    QCoreApplication::instance()->processEvents();
    m_audioOutput->disconnect();
    setPosition(0);
}

// AudioOutput devices (like speaker) will call this function to get new audio data
qint64 MediaPlayer::readData(char* data, qint64 maxlen)
{
    QMutexLocker l(&readMutex);

    // Limit max len
    if(maxlen > MAX_AUDIO_STREAM_SAMPLE_SIZE) maxlen = MAX_AUDIO_STREAM_SAMPLE_SIZE;

    memset(data, 0, maxlen);
    qint64 bytesRead = 0;

    if (m_state == PlaybackState::PlayingState)
    {
        // Copy bytes from buffer into data
        bytesRead = m_output.read(data, maxlen);

        // Emmit newData event, for visualization
        if (maxlen > 0)
        {
            QByteArray buff(data, bytesRead);
            emit newData(buff);
        }

        // If we are at the end of the file, stop
        if (atEnd())
        {
            int timeout = 1; // msecs
            QTimer::singleShot(timeout, this, &MediaPlayer::onAtEnd);
        }
    }

    return bytesRead;
}

qint64 MediaPlayer::writeData(const char* data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);

    return 0;
}

// Start playing audio file
void MediaPlayer::play()
{
    if(m_state == PlaybackState::PlayingState) return;

    if(!m_audioOutput)
        return;

    if(m_state == PlaybackState::PausedState) {
        m_audioOutput->resume();
    } else {
        m_audioOutput->start(this);
    }

    m_state = PlaybackState::PlayingState;
    emit playbackStateChanged(m_state);
}

void MediaPlayer::pause()
{
    if(m_state == PlaybackState::PausedState) return;

    if(!m_audioOutput)
        return;

    m_audioOutput->suspend();

    m_state = PlaybackState::PausedState;
    emit playbackStateChanged(m_state);
}

// Stop playing audio file
void MediaPlayer::stop(bool stopAudioOutput)
{
    if(m_state == PlaybackState::StoppedState) return;

    if(!m_audioOutput)
        return;

    if(stopAudioOutput) m_audioOutput->stop();
    // Clear buffers, avoids pops and clicks when playing after stopping
    this->reset();
    // Fully reset audio output
    clearAudioOutput();
    setupAudioOutput();

    setPosition(0);
    onPositionChanged();

    m_state = PlaybackState::StoppedState;
    emit playbackStateChanged(m_state);
}


void MediaPlayer::clear()
{
    QMutexLocker l(&initMutex);
    clearAudioOutput();
    clearDecoder();
    m_data.clear();
    m_input.close();
    m_output.close();
    isDecodingFinished = false;
    isInited = false;
    bufferUnderrunRetries = 0;
}

// Is at the end of the file
bool MediaPlayer::atEnd() const
{
    return m_output.size()
           && m_output.atEnd()
           && isDecodingFinished;
}

void MediaPlayer::loadMetaData() {
    m_metaData = parseMetaData(m_source);

    setMediaStatus(MediaPlayer::LoadedMedia);
    emit metaDataChanged();
}

void MediaPlayer::setError(Error error)
{
    m_error = error;
    emit errorChanged();
}

void MediaPlayer::setMediaStatus(MediaStatus status)
{
    if(m_status == status) return;
    m_status = status;
    emit mediaStatusChanged(m_status);
}

/////////////////////////////////////////////////////////////////////
// QAudioDecoder logic. These methods are responsible for decoding audio files and putting audio data into stream buffer

// Run when decoder decoded some audio data
void MediaPlayer::bufferReady() // SLOT
{
    const QAudioBuffer &buffer = m_decoder->read();

    const int length = buffer.byteCount();
    const char *data = buffer.constData<char>();

    m_input.write(data, length);

    if(m_status != BufferingMedia) {
        setMediaStatus(BufferingMedia);
    }

    emit bufferProgressChanged(bufferProgress());
}

// Run when decoder finished decoding
void MediaPlayer::finished() // SLOT
{
    isDecodingFinished = true;
    emit bufferProgressChanged(bufferProgress());
    if(m_status != BufferedMedia) {
        setMediaStatus(BufferedMedia);
    }
}

// Handle positionChanged from decoder and adapt to ms
void MediaPlayer::onPositionChanged() // SLOT
{
    m_position = m_output.pos()
                 / (m_format.sampleFormat())
                 / (m_format.sampleRate() / 1000)
                 / (m_format.channelCount());
    emit positionChanged(m_position);
}

void MediaPlayer::onDurationChanged(qint64 duration) // SLOT
{
    if(duration < 0) return;
    emit durationChanged(duration);
}

void MediaPlayer::onDecoderError(QAudioDecoder::Error error)
{
    switch(error) {
    case QAudioDecoder::NoError:
        setError(MediaPlayer::NoError);
        break;
    case QAudioDecoder::ResourceError:
        setError(MediaPlayer::ResourceError);
        break;
    case QAudioDecoder::FormatError:
        setError(MediaPlayer::FormatError);
        break;
    case QAudioDecoder::AccessDeniedError:
        setError(MediaPlayer::AccessDeniedError);
        break;
    case QAudioDecoder::NotSupportedError:
        setError(MediaPlayer::FormatError);
        break;
    }
}

void MediaPlayer::onAtEnd()
{
    // Avoid lock
    if(m_status == MediaStatus::LoadingMedia) return;
    stop(false);
    setMediaStatus(EndOfMedia);
}

/////////////////////////////////////////////////////////////////////

void MediaPlayer::onOutputStateChanged(QAudio::State newState)
{
    QAudio::Error error = m_audioOutput->error();
    if(error != QAudio::NoError) {
        qDebug() << "Audio Ouput Error: " << error;
    }

    switch(newState) {
        case QAudio::IdleState:
            if(error == QAudio::UnderrunError) {

                if(this->bufferUnderrunRetries > MP_MAX_BUFFER_UNDERRUN_RETRIES) {
                    this->stop();
                    qDebug() << "Media Player Buffer Underrun: max retries reached";
                    return;
                }
                this->bufferUnderrunRetries++;

                // Handle race condition on initial loading
                // Retry playing
                qint64 pos = this->position();
                qDebug() << "Retry playing from position: " << pos;

                this->stop();

                // Give some time for the buffer to fill
                int timeout = 200; // msecs
                QTimer::singleShot(timeout, this, [=](){
                    this->play();
                    // Try resuming from previous position
                    this->setPosition(pos);
                });
            }
        break;
        default:
        break;
    }
}

MediaPlayer::PlaybackState MediaPlayer::playbackState() const
{
    return m_state;
}

qint64 MediaPlayer::duration() const
{
    qint64 duration = m_decoder ? m_decoder->duration() : 0;
    return duration;
}

qint64 MediaPlayer::position() const
{
    return m_position;
}

float MediaPlayer::bufferProgress() const
{
    qint64 totalDurationMs = m_decoder->duration();
    if(totalDurationMs < 0) return 100.0;
    qint64 totalExpectedBufferSize = totalDurationMs
                                     * (m_format.sampleFormat())
                                     * (m_format.sampleRate() / 1000)
                                     * (m_format.channelCount());
    qsizetype currentBufferSize = m_data.size();
    float progress = (float (currentBufferSize) / float (totalExpectedBufferSize)) * 100.0;
    progress = progress > 100.00 ? 100.00 : progress;
    return progress;
}

MediaPlayer::MediaStatus MediaPlayer::mediaStatus() const
{
    return m_status;
}

float MediaPlayer::volume() const
{
    return m_volume;
}

QMediaMetaData MediaPlayer::metaData() const
{
    return m_metaData;
}

MediaPlayer::Error MediaPlayer::error() const
{
    return m_error;
}

QString MediaPlayer::errorString() const
{
    QString errorStr = "";
    switch(m_error) {
    case NoError:
        break;
    case ResourceError:
        errorStr = "Resource Error";
        if(m_decoder) errorStr.append(": ").append(m_decoder->errorString());
        break;
    case FormatError:
        return "Format Error";
        if(m_decoder) errorStr.append(": ").append(m_decoder->errorString());
        break;
    case NetworkError:
        return "Network Error";
        if(m_decoder) errorStr.append(": ").append(m_decoder->errorString());
        break;
    case AccessDeniedError:
        return "Access Denied";
        if(m_decoder) errorStr.append(": ").append(m_decoder->errorString());
        break;
    }
    return errorStr;
}

QAudioFormat MediaPlayer::format()
{
    return m_format;
}


void MediaPlayer::setSource(const QUrl &source)
{
    setMediaStatus(MediaPlayer::LoadingMedia);
    if(m_state != PlaybackState::StoppedState) {
        stop();
    }
    clear();
    QAudioDevice info(QMediaDevices::defaultAudioOutput());
    QAudioFormat format = info.preferredFormat();
    if(!format.isValid()) {
        qDebug() << "WARNING: Audio format in default audio output is not defined, using defaults";
        format.setSampleFormat(QAudioFormat::Int16);
        format.setSampleRate(DEFAULT_SAMPLE_RATE);
        format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
        format.setChannelCount(2);
    }
    init(format);
    m_source = source;
    m_decoder->setSource(m_source);
    m_decoder->start();
    loadMetaData();
}

void MediaPlayer::clearSource()
{
    setMediaStatus(MediaPlayer::NoMedia);
    if(m_state != PlaybackState::StoppedState) {
        stop();
    }
    clear();
    if(m_audioOutput) {
        delete m_audioOutput;
        m_audioOutput = nullptr;
    }
    m_metaData = QMediaMetaData{};
    emit metaDataChanged();
    emit durationChanged(0);
}

void MediaPlayer::setPosition(qint64 position)
{
    // Don't let set the possition while media is still buffering, avoid noise error
    if(m_status == MediaStatus::BufferingMedia) return;
    const qsizetype currentBufferSize = m_data.size();
    qint64 target = position
                       * (m_format.sampleFormat())
                       * (m_format.sampleRate() / 1000)
                       * (m_format.channelCount());

    if(target >= currentBufferSize) target = currentBufferSize - 1;
    if(target < 0) target = 0;

    m_output.seek(target);
}

void MediaPlayer::setVolume(float volume)
{
    m_volume = volume;
    if(m_audioOutput) m_audioOutput->setVolume(m_volume);
}

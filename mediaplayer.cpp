#include "mediaplayer.h"
#include "qurl.h"
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>
#include <QDebug>
#include <QMediaDevices>
#import <QCoreApplication>

MediaPlayer::MediaPlayer(QObject *parent) :
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
    m_format = format;

    // Initialize buffers
    if (!m_output.open(QIODevice::ReadOnly) || !m_input.open(QIODevice::WriteOnly))
    {
        return false;
    }

    clear();
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
    connect(this, &MediaPlayer::newData, this, &MediaPlayer::onPositionChanged);
}

void MediaPlayer::setupAudioOutput()
{
    m_audioOutput = new QAudioSink(m_format, this);
    //connect(m_audioOutput, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)));
    m_audioOutput->start(this);
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
    memset(data, 0, maxlen);

    if (m_state == PlaybackState::PlayingState)
    {
        // Copy bytes from buffer into data
        m_output.read(data, maxlen);

        // Emmit newData event, for visualization
        if (maxlen > 0)
        {
            QByteArray buff(data, maxlen);
            emit newData(buff);
        }

        // If we are at the end of the file, stop
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
    if(m_state == PlaybackState::PlayingState) return;

    if(m_state == PlaybackState::PausedState) {
        m_decoder->start();
        m_audioOutput->resume();
    } else {
        clear();
        QAudioDevice info(QMediaDevices::defaultAudioOutput());
        QAudioFormat format = info.preferredFormat();
        init(format);
        m_decoder->setSource(m_source);
        m_decoder->start();
    }

    m_state = PlaybackState::PlayingState;
    emit playbackStateChanged(m_state);
}

void MediaPlayer::pause()
{
    if(m_state == PlaybackState::PausedState) return;

    m_decoder->stop();
    m_audioOutput->suspend();

    m_state = PlaybackState::PausedState;
    emit playbackStateChanged(m_state);
}

// Stop play audio file
void MediaPlayer::stop()
{
    if(m_state == PlaybackState::StoppedState) return;

    m_decoder->stop();
    m_audioOutput->stop();

    clear();
    m_state = PlaybackState::StoppedState;
    emit playbackStateChanged(m_state);
}


void MediaPlayer::clear()
{
    clearAudioOutput();
    clearDecoder();
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

void MediaPlayer::parseMetaData() {
    TagLib::FileRef f(m_source.toLocalFile().toLocal8Bit().data());

    if(!f.isNull() && f.tag()) {
        TagLib::Tag *tag = f.tag();

        QString title = QString::fromStdString(tag->title().toCString(true));
        QString albumTitle = QString::fromStdString(tag->album().toCString(true));
        QString artist = QString::fromStdString(tag->artist().toCString(true));
        QString comment = QString::fromStdString(tag->comment().toCString(true));
        QString genre = QString::fromStdString(tag->genre().toCString(true));
        qint64 track = tag->track();
        qint64 year = tag->year();

        m_metaData = QMediaMetaData{};
        m_metaData.insert(QMediaMetaData::Title, title);
        m_metaData.insert(QMediaMetaData::AlbumTitle, albumTitle);
        m_metaData.insert(QMediaMetaData::AlbumArtist, artist);
        m_metaData.insert(QMediaMetaData::Comment, comment);
        m_metaData.insert(QMediaMetaData::Genre, genre);
        m_metaData.insert(QMediaMetaData::TrackNumber, track);
        m_metaData.insert(QMediaMetaData::Url, m_source);
        m_metaData.insert(QMediaMetaData::Date, year);
    }

    if(!f.isNull() && f.audioProperties()) {
        TagLib::AudioProperties *properties = f.audioProperties();

        qint64 duration = properties->lengthInMilliseconds();
        qint64 bitrate = properties->bitrate() * 1000;
        qint64 sampleRate = properties->sampleRate();

        //m_metaData.insert(QMediaMetaData::MediaType, artist); // TODO
        m_metaData.insert(QMediaMetaData::AudioBitRate, bitrate);
        m_metaData.insert(QMediaMetaData::AudioCodec, sampleRate); // Using AudioCodec as sample rate for now
        m_metaData.insert(QMediaMetaData::Duration, duration);
    }

    emit metaDataChanged();
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

    emit bufferProgressChanged(bufferProgress());
}

// Run when decoder finished decoding
void MediaPlayer::finished() // SLOT
{
    isDecodingFinished = true;
    emit bufferProgressChanged(bufferProgress());
}

// Handle positionChanged from decoder and adapt to ms
void MediaPlayer::onPositionChanged() // SLOT
{
    m_position = m_output.pos()
                 / (m_format.sampleFormat())
                 / (m_format.sampleRate() / 1000)
                 / (m_format.channelCount());
    //qDebug() << "Position: " << m_position;
    emit positionChanged(m_position);
}

void MediaPlayer::onDurationChanged(qint64 duration) // SLOT
{
    if(duration < 0) return;
    emit durationChanged(duration);
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
    qint64 duration = m_decoder ? m_decoder->duration() : 0;
    //qDebug() << "Duration: " << duration;
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
    //qDebug() << "totalExpectedBufferSize: " << totalExpectedBufferSize;
    //qDebug() << "currentBufferSize: " << currentBufferSize;
    float progress = (float (currentBufferSize) / float (totalExpectedBufferSize)) * 100.0;
    progress = progress > 100.00 ? 100.00 : progress;
    //qDebug() << "progress: " << progress;
    return progress;
}

MediaPlayer::MediaStatus MediaPlayer::mediaStatus() const
{
    // TODO
    return MediaStatus::BufferingMedia;
}

float MediaPlayer::volume() const
{
    return m_volume;
}

QMediaMetaData MediaPlayer::metaData() const
{
    return m_metaData;
}

void MediaPlayer::setSource(const QUrl &source)
{
    if(m_state != PlaybackState::StoppedState) {
        stop();
    }
    qInfo() << source.toString();
    m_source = source;
    parseMetaData();
}

void MediaPlayer::setPosition(qint64 position)
{
    const qsizetype currentBufferSize = m_data.size();
    qint64 target = position
                       * (m_format.sampleFormat())
                       * (m_format.sampleRate() / 1000)
                       * (m_format.channelCount());

    if(target >= currentBufferSize) target = currentBufferSize - 1;

    m_output.seek(target);
}

void MediaPlayer::setVolume(float volume)
{
    m_volume = volume;
    if(m_audioOutput) m_audioOutput->setVolume(m_volume);
}

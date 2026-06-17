#include "mediaplayer.h"

#include "util.h"

#include <QAudioOutput>
#include <QMediaPlayer>
#include <QMetaObject>
#include <QTimer>

class MediaPlayerBackend : public QObject
{
    Q_OBJECT
public:
    explicit MediaPlayerBackend(QObject *parent = nullptr)
        : QObject(parent)
    {
        m_audioOutput = new QAudioOutput(this);
        m_audioOutput->setVolume(1.0);

        m_player = new QMediaPlayer(this);
        m_player->setAudioOutput(m_audioOutput);

        m_positionTimer = new QTimer(this);
        m_positionTimer->setInterval(200);
        connect(m_positionTimer, &QTimer::timeout, this, [this]() {
            emit positionChanged(m_player->position());
        });

        connect(m_player, &QMediaPlayer::playbackStateChanged,
                this, [this](QMediaPlayer::PlaybackState state) {
                    emit playbackStateChanged(static_cast<int>(state));
                });

        connect(m_player, &QMediaPlayer::mediaStatusChanged,
                this, [this](QMediaPlayer::MediaStatus status) {
                    emit mediaStatusChanged(static_cast<int>(status));
                });

        connect(m_player, &QMediaPlayer::durationChanged, this, &MediaPlayerBackend::durationChanged);
        connect(m_player, &QMediaPlayer::positionChanged, this, &MediaPlayerBackend::positionChanged);
        connect(m_player, &QMediaPlayer::bufferProgressChanged, this, &MediaPlayerBackend::bufferProgressChanged);

        connect(m_audioOutput, &QAudioOutput::volumeChanged,
                this, &MediaPlayerBackend::volumeChanged);

        connect(m_player, &QMediaPlayer::metaDataChanged, this, [this]() {
            emit metaDataChanged(m_player->metaData());
        });

        connect(m_player, &QMediaPlayer::errorChanged, this, [this]() {
            emit errorChanged(static_cast<int>(m_player->error()), m_player->errorString());
        });
    }

public slots:
    void play()
    {
        m_player->play();
        m_positionTimer->start();
    }

    void pause()
    {
        m_player->pause();
        m_positionTimer->stop();
    }

    void stop()
    {
        m_player->stop();
        m_positionTimer->stop();
    }

    void setSource(const QUrl &source)
    {
        m_player->setSource(source);
    }

    void clearSource()
    {
        m_player->setSource(QUrl());
        m_positionTimer->stop();
    }

    void setPosition(qint64 position)
    {
        m_player->setPosition(position);
    }

    void setVolume(float volume)
    {
        m_audioOutput->setVolume(volume);
    }

signals:
    void playbackStateChanged(int state);
    void mediaStatusChanged(int status);
    void durationChanged(qint64 duration);
    void positionChanged(qint64 position);
    void bufferProgressChanged(float progress);
    void volumeChanged(float volume);
    void metaDataChanged(const QMediaMetaData &metaData);
    void errorChanged(int error, const QString &errorString);

private:
    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
    QTimer *m_positionTimer = nullptr;
};

MediaPlayer::MediaPlayer(QObject *parent)
    : QObject(parent)
{
    m_backend = new MediaPlayerBackend();

    connect(m_backend, &MediaPlayerBackend::playbackStateChanged,
        this, &MediaPlayer::handleBackendPlaybackStateChanged);
    connect(m_backend, &MediaPlayerBackend::mediaStatusChanged,
        this, &MediaPlayer::handleBackendMediaStatusChanged);
    connect(m_backend, &MediaPlayerBackend::durationChanged,
        this, &MediaPlayer::handleBackendDurationChanged);
    connect(m_backend, &MediaPlayerBackend::positionChanged,
        this, &MediaPlayer::handleBackendPositionChanged);
    connect(m_backend, &MediaPlayerBackend::bufferProgressChanged,
        this, &MediaPlayer::handleBackendBufferProgressChanged);
    connect(m_backend, &MediaPlayerBackend::volumeChanged,
        this, &MediaPlayer::handleBackendVolumeChanged);
    connect(m_backend, &MediaPlayerBackend::metaDataChanged,
        this, &MediaPlayer::handleBackendMetaDataChanged);
    connect(m_backend, &MediaPlayerBackend::errorChanged,
        this, &MediaPlayer::handleBackendErrorChanged);

    m_format.setSampleFormat(QAudioFormat::Int16);
    m_format.setSampleRate(DEFAULT_SAMPLE_RATE);
    m_format.setChannelConfig(QAudioFormat::ChannelConfigStereo);
    m_format.setChannelCount(2);
}

MediaPlayer::~MediaPlayer()
{
    delete m_backend;
    m_backend = nullptr;
}

void MediaPlayer::play()
{
    m_backend->play();
}

void MediaPlayer::pause()
{
    m_backend->pause();
}

void MediaPlayer::stop(bool)
{
    m_backend->stop();
}

MediaPlayer::PlaybackState MediaPlayer::playbackState() const
{
    return m_state;
}

qint64 MediaPlayer::duration() const
{
    return m_duration;
}

qint64 MediaPlayer::position() const
{
    return m_position;
}

float MediaPlayer::bufferProgress() const
{
    return m_bufferProgress;
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
    return m_errorString;
}

QAudioFormat MediaPlayer::format()
{
    return m_format;
}

bool MediaPlayer::isMissingTitle(const QMediaMetaData &metaData)
{
    return metaData.value(QMediaMetaData::Title).toString().trimmed().isEmpty();
}

bool MediaPlayer::isMissingAlbumTitle(const QMediaMetaData &metaData)
{
    return metaData.value(QMediaMetaData::AlbumTitle).toString().trimmed().isEmpty();
}

bool MediaPlayer::isMissingAlbumArtist(const QMediaMetaData &metaData)
{
    return metaData.value(QMediaMetaData::AlbumArtist).toString().trimmed().isEmpty();
}

bool MediaPlayer::isMissingGenre(const QMediaMetaData &metaData)
{
    return metaData.value(QMediaMetaData::Genre).toString().trimmed().isEmpty();
}

bool MediaPlayer::isMissingTrackNumber(const QMediaMetaData &metaData)
{
    return metaData.value(QMediaMetaData::TrackNumber).toInt() <= 0;
}

bool MediaPlayer::isMissingDate(const QMediaMetaData &metaData)
{
    return !metaData.value(QMediaMetaData::Date).isValid() || metaData.value(QMediaMetaData::Date).isNull();
}

bool MediaPlayer::isMissingBitrate(const QMediaMetaData &metaData)
{
    return metaData.value(QMediaMetaData::AudioBitRate).toInt() <= 0;
}

bool MediaPlayer::isMissingSampleRate(const QMediaMetaData &metaData)
{
    bool ok = false;
    const int sampleRate = metaData.value(QMediaMetaData::Comment).toString().toInt(&ok);
    return !ok || sampleRate <= 0;
}

bool MediaPlayer::isMissingDuration(const QMediaMetaData &metaData)
{
    return metaData.value(QMediaMetaData::Duration).toLongLong() <= 0;
}

MediaPlayer::PlaybackState MediaPlayer::mapPlaybackState(int state)
{
    switch (static_cast<QMediaPlayer::PlaybackState>(state)) {
    case QMediaPlayer::StoppedState:
        return StoppedState;
    case QMediaPlayer::PlayingState:
        return PlayingState;
    case QMediaPlayer::PausedState:
        return PausedState;
    }
    return StoppedState;
}

MediaPlayer::MediaStatus MediaPlayer::mapMediaStatus(int status)
{
    switch (static_cast<QMediaPlayer::MediaStatus>(status)) {
    case QMediaPlayer::NoMedia:
        return NoMedia;
    case QMediaPlayer::LoadingMedia:
        return LoadingMedia;
    case QMediaPlayer::LoadedMedia:
        return LoadedMedia;
    case QMediaPlayer::StalledMedia:
        return StalledMedia;
    case QMediaPlayer::BufferingMedia:
        return BufferingMedia;
    case QMediaPlayer::BufferedMedia:
        return BufferedMedia;
    case QMediaPlayer::EndOfMedia:
        return EndOfMedia;
    case QMediaPlayer::InvalidMedia:
        return InvalidMedia;
    }
    return InvalidMedia;
}

MediaPlayer::Error MediaPlayer::mapError(int error)
{
    switch (static_cast<QMediaPlayer::Error>(error)) {
    case QMediaPlayer::NoError:
        return NoError;
    case QMediaPlayer::ResourceError:
        return ResourceError;
    case QMediaPlayer::FormatError:
        return FormatError;
    case QMediaPlayer::NetworkError:
        return NetworkError;
    case QMediaPlayer::AccessDeniedError:
        return AccessDeniedError;
    }
    return FormatError;
}

void MediaPlayer::handleBackendPlaybackStateChanged(int state)
{
    const PlaybackState mapped = mapPlaybackState(state);
    if (mapped == m_state) {
        return;
    }
    m_state = mapped;
    emit playbackStateChanged(m_state);
}

void MediaPlayer::handleBackendMediaStatusChanged(int status)
{
    const MediaStatus mapped = mapMediaStatus(status);
    if (mapped == m_status) {
        return;
    }
    m_status = mapped;
    emit mediaStatusChanged(m_status);
}

void MediaPlayer::handleBackendDurationChanged(qint64 duration)
{
    // Some backends briefly report 0 during pause/stop transitions.
    // Keep the last known valid duration while a source is still loaded.
    if (duration <= 0 && m_hasSource && m_duration > 0) {
        return;
    }

    if (m_duration == duration) {
        return;
    }
    m_duration = duration;
    emit durationChanged(m_duration);
}

void MediaPlayer::handleBackendPositionChanged(qint64 position)
{
    if (m_position == position) {
        return;
    }
    m_position = position;
    emit positionChanged(m_position);
}

void MediaPlayer::handleBackendBufferProgressChanged(float progress)
{
    // Keep API behavior: progress in percentage [0,100].
    m_bufferProgress = progress * 100.0f;
    emit bufferProgressChanged(m_bufferProgress);
}

void MediaPlayer::handleBackendVolumeChanged(float volume)
{
    if (qFuzzyCompare(m_volume, volume)) {
        return;
    }
    m_volume = volume;
    emit volumeChanged(m_volume);
}

void MediaPlayer::handleBackendMetaDataChanged(const QMediaMetaData &metaData)
{
    m_metaData = metaData;

    if (m_hasSource && m_sourceUrl.isValid() && !m_sourceUrl.isEmpty()) {
        const bool needsFallback = isMissingTitle(m_metaData)
                                   || isMissingAlbumTitle(m_metaData)
                                   || isMissingAlbumArtist(m_metaData)
                                   || isMissingGenre(m_metaData)
                                   || isMissingTrackNumber(m_metaData)
                                   || isMissingDate(m_metaData)
                                   || isMissingBitrate(m_metaData)
                                   || isMissingSampleRate(m_metaData)
                                   || isMissingDuration(m_metaData);

        if (needsFallback) {
            const QMediaMetaData taglibMetaData = parseMetaData(m_sourceUrl);

            if (isMissingTitle(m_metaData)) {
                const QString title = taglibMetaData.value(QMediaMetaData::Title).toString().trimmed();
                if (!title.isEmpty()) {
                    m_metaData.insert(QMediaMetaData::Title, title);
                }
            }

            if (isMissingAlbumTitle(m_metaData)) {
                const QString albumTitle = taglibMetaData.value(QMediaMetaData::AlbumTitle).toString().trimmed();
                if (!albumTitle.isEmpty()) {
                    m_metaData.insert(QMediaMetaData::AlbumTitle, albumTitle);
                }
            }

            if (isMissingAlbumArtist(m_metaData)) {
                const QString albumArtist = taglibMetaData.value(QMediaMetaData::AlbumArtist).toString().trimmed();
                if (!albumArtist.isEmpty()) {
                    m_metaData.insert(QMediaMetaData::AlbumArtist, albumArtist);
                }
            }

            if (isMissingGenre(m_metaData)) {
                const QString genre = taglibMetaData.value(QMediaMetaData::Genre).toString().trimmed();
                if (!genre.isEmpty()) {
                    m_metaData.insert(QMediaMetaData::Genre, genre);
                }
            }

            if (isMissingTrackNumber(m_metaData)) {
                const int trackNumber = taglibMetaData.value(QMediaMetaData::TrackNumber).toInt();
                if (trackNumber > 0) {
                    m_metaData.insert(QMediaMetaData::TrackNumber, trackNumber);
                }
            }

            if (isMissingDate(m_metaData)) {
                const QVariant date = taglibMetaData.value(QMediaMetaData::Date);
                if (date.isValid() && !date.isNull()) {
                    m_metaData.insert(QMediaMetaData::Date, date);
                }
            }

            if (isMissingBitrate(m_metaData)) {
                const int bitrate = taglibMetaData.value(QMediaMetaData::AudioBitRate).toInt();
                if (bitrate > 0) {
                    m_metaData.insert(QMediaMetaData::AudioBitRate, bitrate);
                }
            }

            if (isMissingSampleRate(m_metaData)) {
                bool ok = false;
                const int sampleRate = taglibMetaData.value(QMediaMetaData::Comment).toString().toInt(&ok);
                if (ok && sampleRate > 0) {
                    m_metaData.insert(QMediaMetaData::Comment, QString::number(sampleRate));
                }
            }

            if (isMissingDuration(m_metaData)) {
                const qint64 duration = taglibMetaData.value(QMediaMetaData::Duration).toLongLong();
                if (duration > 0) {
                    m_metaData.insert(QMediaMetaData::Duration, duration);
                }
            }

            if (!m_metaData.value(QMediaMetaData::Url).isValid() || m_metaData.value(QMediaMetaData::Url).isNull()) {
                const QUrl url = taglibMetaData.value(QMediaMetaData::Url).toUrl();
                if (url.isValid() && !url.isEmpty()) {
                    m_metaData.insert(QMediaMetaData::Url, url);
                }
            }
        }
    }

    emit metaDataChanged();
}

void MediaPlayer::handleBackendErrorChanged(int error, const QString &errorString)
{
    const Error mapped = mapError(error);
    const bool changed = (mapped != m_error) || (errorString != m_errorString);
    m_error = mapped;
    m_errorString = errorString;
    if (changed) {
        emit errorChanged();
    }
}

void MediaPlayer::setSource(const QUrl &source)
{
    m_hasSource = source.isValid() && !source.isEmpty();
    m_sourceUrl = source;
    m_backend->setSource(source);
}

void MediaPlayer::clearSource()
{
    m_hasSource = false;
    m_sourceUrl = QUrl();
    m_backend->clearSource();

    m_metaData = QMediaMetaData{};
    m_duration = 0;
    m_position = 0;
    m_bufferProgress = 0.0f;
    m_status = NoMedia;
    m_state = StoppedState;
    m_error = NoError;
    m_errorString.clear();

    emit metaDataChanged();
    emit durationChanged(0);
    emit positionChanged(0);
    emit mediaStatusChanged(NoMedia);
    emit playbackStateChanged(StoppedState);
    emit bufferProgressChanged(0.0f);
    emit errorChanged();
}

void MediaPlayer::setPosition(qint64 position)
{
    m_backend->setPosition(position);
}

void MediaPlayer::setVolume(float volume)
{
    if (qFuzzyCompare(m_volume, volume)) {
        return;
    }

    m_volume = volume;
    emit volumeChanged(m_volume);

    m_backend->setVolume(volume);
}

#include "mediaplayer.moc"

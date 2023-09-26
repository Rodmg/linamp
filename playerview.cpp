#include "playerview.h"
#include "ui_playerview.h"

#include "playlistmodel.h"
#include "qmediaplaylist.h"

#include <QApplication>
#include <QAudioDevice>
#include <QAudioOutput>
#include <QDir>
#include <QFileDialog>
#include <QMediaDevices>
#include <QMediaFormat>
#include <QMediaMetaData>
#include <QMessageBox>
#include <QStandardPaths>
#include <QFontDatabase>

PlayerView::PlayerView(QWidget *parent, PlaylistModel *playlistModel) :
    QWidget(parent),
    ui(new Ui::PlayerView)
{
    // Load custom fonts
    QFontDatabase::addApplicationFont(":/assets/bignumbers.ttf");

    // Setup UI
    ui->setupUi(this);

    //! [create-objs]
    m_player = new MediaPlayer(this);
    //! [create-objs]
    connect(m_player, &MediaPlayer::durationChanged, this, &PlayerView::durationChanged);
    connect(m_player, &MediaPlayer::positionChanged, this, &PlayerView::positionChanged);
    //connect(m_player, QOverload<>::of(&MediaPlayer::metaDataChanged), this,
    //        &PlayerView::metaDataChanged);
    //connect(m_player, &MediaPlayer::mediaStatusChanged, this, &PlayerView::statusChanged);
    connect(m_player, &MediaPlayer::bufferProgressChanged, this, &PlayerView::bufferingProgress);
    //connect(m_player, &MediaPlayer::errorChanged, this, &PlayerView::displayErrorMessage);

    m_playlistModel = playlistModel;
    m_playlist = m_playlistModel->playlist();
    //! [2]
    connect(m_playlist, &QMediaPlaylist::currentIndexChanged, this,
            &PlayerView::playlistPositionChanged);

    // duration slider and label
    ui->posBar->setRange(0, m_player->duration());
    connect(ui->posBar, &QSlider::sliderMoved, this, &PlayerView::seek);

    // controls
    connect(ui->openButton, &QPushButton::clicked, this, &PlayerView::open);

    // Set volume slider
    ui->volumeSlider->setRange(0, 100);
    this->setVolumeSlider(m_player->volume());

    // Set Balance Slider
    ui->balanceSlider->setRange(0, 100);
    ui->balanceSlider->setValue(50);

    // Reset time counter
    ui->progressTimeLabel->setText("");

    // Set play status icon
    setPlaybackState(m_player->playbackState());

    connect(ui->playButton, &QPushButton::clicked, this, &PlayerView::playClicked);
    connect(ui->pauseButton, &QPushButton::clicked, this, &PlayerView::pauseClicked);
    connect(ui->stopButton, &QPushButton::clicked, this, &PlayerView::stopClicked);
    connect(ui->nextButton, &QPushButton::clicked, m_playlist, &QMediaPlaylist::next);
    connect(ui->backButton, &QPushButton::clicked, this, &PlayerView::previousClicked);
    connect(ui->volumeSlider, &QSlider::valueChanged, this, &PlayerView::volumeChanged);
    connect(ui->playlistButton, &QCheckBox::clicked, this, &PlayerView::showPlaylistClicked);

    connect(m_player, &MediaPlayer::playbackStateChanged, this, &PlayerView::setPlaybackState);
    connect(m_player, &MediaPlayer::volumeChanged, this, &PlayerView::setVolumeSlider);

    if (!isPlayerAvailable()) {
        QMessageBox::warning(this, tr("Service not available"),
                             tr("The QMediaPlayer object does not have a valid service.\n"
                                "Please check the media service plugins are installed."));
        // Should disable ui here...
    }

    metaDataChanged();
}

PlayerView::~PlayerView()
{
    delete ui;
}

bool PlayerView::isPlayerAvailable() const
{
    return true; //m_player->isAvailable();
}

void PlayerView::open()
{
    QFileDialog fileDialog(this);
    fileDialog.setNameFilter(tr("Audio (*.mp3 *.flac *.m4a *.ogg *.wma *.wav *.m3u)"));
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setFileMode(QFileDialog::ExistingFiles);
    fileDialog.setWindowTitle(tr("Open Files"));
    fileDialog.setDirectory(QStandardPaths::standardLocations(QStandardPaths::MusicLocation)
                                .value(0, QDir::homePath()));
    if (fileDialog.exec() == QDialog::Accepted)
        addToPlaylist(fileDialog.selectedUrls());
}

static bool isPlaylist(const QUrl &url) // Check for ".m3u" playlists.
{
    if (!url.isLocalFile())
        return false;
    const QFileInfo fileInfo(url.toLocalFile());
    return fileInfo.exists()
           && !fileInfo.suffix().compare(QLatin1String("m3u"), Qt::CaseInsensitive);
}

void PlayerView::addToPlaylist(const QList<QUrl> &urls)
{
    const int previousMediaCount = m_playlist->mediaCount();
    for (auto &url : urls) {
        if (isPlaylist(url))
            m_playlist->load(url);
        else
            m_playlist->addMedia(url);
    }
    if (m_playlist->mediaCount() > previousMediaCount) {
        auto index = m_playlistModel->index(previousMediaCount, 0);
        jump(index);
    }
}

void PlayerView::durationChanged(qint64 duration)
{
    m_duration = duration / 1000;
    ui->posBar->setMaximum(duration);
}

void PlayerView::positionChanged(qint64 progress)
{
    if (!ui->posBar->isSliderDown())
        ui->posBar->setValue(progress);

    updateDurationInfo(progress / 1000);
}

void PlayerView::metaDataChanged()
{
    /*auto metaData = m_player->metaData();

    // Generate track info string
    QString artist = metaData.value(QMediaMetaData::AlbumArtist).toString().toUpper();
    QString album = metaData.value(QMediaMetaData::AlbumTitle).toString().toUpper();
    QString title = metaData.value(QMediaMetaData::Title).toString().toUpper();

    //  Calculate duration
    qint64 duration = metaData.value(QMediaMetaData::Duration).toLongLong()/1000;
    QTime totalTime((duration / 3600) % 60, (duration / 60) % 60, duration % 60,
                    (duration * 1000) % 1000);
    QString format = "mm:ss";
    if (duration > 3600)
        format = "hh:mm:ss";
    QString durationStr = totalTime.toString(format);

    QString trackInfo = "";

    if(artist.length()) trackInfo.append(QString("%1 - ").arg(artist));
    if(album.length()) trackInfo.append(QString("%1 - ").arg(album));
    if(title.length()) trackInfo.append(title);
    if(totalTime > QTime(0, 0, 0)) trackInfo.append(QString(" (%1)").arg(durationStr));

    setTrackInfo(trackInfo);

    // Set kbps
    int bitrate = metaData.value(QMediaMetaData::AudioBitRate).toInt()/1000;
    ui->kbpsValueLabel->setText(bitrate > 0 ? QString::number(bitrate) : "");

    // Set kHz TODO
    QString khz = metaData.value(QMediaMetaData::AudioCodec).toString();
    ui->khzValueLabel->setText(khz);*/
}

QString PlayerView::trackName(const QMediaMetaData &metaData, int index)
{
    QString name;
    QString title = metaData.stringValue(QMediaMetaData::Title);
    QLocale::Language lang = metaData.value(QMediaMetaData::Language).value<QLocale::Language>();

    if (title.isEmpty()) {
        if (lang == QLocale::Language::AnyLanguage)
            name = tr("Track %1").arg(index + 1);
        else
            name = QLocale::languageToString(lang);
    } else {
        if (lang == QLocale::Language::AnyLanguage)
            name = title;
        else
            name = QString("%1 - [%2]").arg(title, QLocale::languageToString(lang));
    }
    return name;
}

void PlayerView::previousClicked()
{
    // Go to previous track if we are within the first 5 seconds of playback
    // Otherwise, seek to the beginning.
    if (m_player->position() <= 5000) {
        m_playlist->previous();
    } else {
        m_player->setPosition(0);
    }
}

void PlayerView::playClicked()
{
    shouldBePlaying = true;
    m_player->play();
}

void PlayerView::pauseClicked()
{
    shouldBePlaying = false;
    m_player->pause();
}

void PlayerView::stopClicked()
{
    shouldBePlaying = false;
    m_player->stop();
}

void PlayerView::jump(const QModelIndex &index)
{
    if (index.isValid()) {
        m_playlist->setCurrentIndex(index.row());
        shouldBePlaying = true;
        m_player->play();
    }
}

void PlayerView::playlistPositionChanged(int currentItem)
{
    m_player->setSource(m_playlist->currentMedia());

    if (shouldBePlaying) {
        m_player->play();
    }
}

void PlayerView::seek(int mseconds)
{
    m_player->setPosition(mseconds);
}

void PlayerView::statusChanged(MediaPlayer::MediaStatus status)
{
    handleCursor(status);

    // handle status message
    switch (status) {
    case MediaPlayer::NoMedia:
    case MediaPlayer::LoadedMedia:
        setStatusInfo(QString());
        break;
    case MediaPlayer::LoadingMedia:
        setStatusInfo(tr("Loading..."));
        break;
    case MediaPlayer::BufferingMedia:
    case MediaPlayer::BufferedMedia:
        setStatusInfo(tr("Buffering %1%").arg(qRound(m_player->bufferProgress() * 100.)));
        break;
    case MediaPlayer::StalledMedia:
        setStatusInfo(tr("Stalled %1%").arg(qRound(m_player->bufferProgress() * 100.)));
        break;
    case MediaPlayer::EndOfMedia:
        QApplication::alert(this);
        m_playlist->next();
        break;
    case MediaPlayer::InvalidMedia:
        displayErrorMessage();
        break;
    }
}

void PlayerView::handleCursor(MediaPlayer::MediaStatus status)
{
#ifndef QT_NO_CURSOR
    if (status == MediaPlayer::LoadingMedia || status == MediaPlayer::BufferingMedia
        || status == MediaPlayer::StalledMedia)
        setCursor(QCursor(Qt::BusyCursor));
    else
        unsetCursor();
#endif
}

void PlayerView::bufferingProgress(float progress)
{
    if (m_player->mediaStatus() == MediaPlayer::StalledMedia)
        setStatusInfo(tr("Stalled %1%").arg(qRound(progress * 100.)));
    else
        setStatusInfo(tr("Buffering %1%").arg(qRound(progress * 100.)));
}

void PlayerView::setTrackInfo(const QString &info)
{
    m_trackInfo = info;

    ui->songInfoLabel->setText(info);

    if (!m_statusInfo.isEmpty())
        setWindowTitle(QString("%1 | %2").arg(m_trackInfo, m_statusInfo));
    else
        setWindowTitle(m_trackInfo);
}

void PlayerView::setStatusInfo(const QString &info)
{
    m_statusInfo = info;

    if (!m_statusInfo.isEmpty())
        setWindowTitle(QString("%1 | %2").arg(m_trackInfo, m_statusInfo));
    else
        setWindowTitle(m_trackInfo);
}

void PlayerView::displayErrorMessage()
{
    /*if (m_player->error() == MediaPlayer::NoError)
        return;
    setStatusInfo(m_player->errorString());*/
}

void PlayerView::updateDurationInfo(qint64 currentInfo)
{
    QString tStr, tDisplayStr;
    if (currentInfo || m_duration) {
        QTime currentTime((currentInfo / 3600) % 60, (currentInfo / 60) % 60, currentInfo % 60,
                          (currentInfo * 1000) % 1000);
        QTime totalTime((m_duration / 3600) % 60, (m_duration / 60) % 60, m_duration % 60,
                        (m_duration * 1000) % 1000);
        QString format = "mm:ss";
        if (m_duration > 3600)
            format = "hh:mm:ss";
        tStr = currentTime.toString(format) + " / " + totalTime.toString(format);
        format = "m ss";
        tDisplayStr = currentTime.toString(format);
    }
    ui->progressTimeLabel->setText(tDisplayStr);
}

void PlayerView::setVolumeSlider(float volume)
{
    qreal logarithmicVolume = QAudio::convertVolume(volume, QAudio::LinearVolumeScale,
                                                    QAudio::LogarithmicVolumeScale);

    ui->volumeSlider->setValue(qRound(logarithmicVolume * 100));
}

void PlayerView::volumeChanged()
{
    qreal linearVolume =
        QAudio::convertVolume(ui->volumeSlider->value() / qreal(100),
                              QAudio::LogarithmicVolumeScale, QAudio::LinearVolumeScale);

    m_player->setVolume(linearVolume);
}

void PlayerView::setPlaybackState(MediaPlayer::PlaybackState state)
{
    QString imageSrc;
    switch(state) {
    case MediaPlayer::StoppedState:
        imageSrc = ":/assets/status_stopped.png";
        break;
    case MediaPlayer::PlayingState:
        imageSrc = ":/assets/status_playing.png";
        break;
    case MediaPlayer::PausedState:
        imageSrc = ":/assets/status_paused.png";
        break;
    }

    // Set play status icon
    QPixmap image;
    image.load(imageSrc);
    QGraphicsScene *scene = new QGraphicsScene(this);
    scene->addPixmap(image);
    scene->setSceneRect(image.rect());
    ui->playStatusIcon->setScene(scene);
}

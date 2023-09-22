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

PlayerView::PlayerView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlayerView)
{
    // Load custom fonts
    QFontDatabase::addApplicationFont(":/assets/Minecraft.ttf");
    QFontDatabase::addApplicationFont(":/assets/Winamp.ttf");
    QFontDatabase::addApplicationFont(":/assets/LED_LCD_123.ttf");
    QFontDatabase::addApplicationFont(":/assets/bignumbers.ttf");

    // Setup UI
    ui->setupUi(this);

    //! [create-objs]
    m_player = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);
    //! [create-objs]
    connect(m_player, &QMediaPlayer::durationChanged, this, &PlayerView::durationChanged);
    connect(m_player, &QMediaPlayer::positionChanged, this, &PlayerView::positionChanged);
    connect(m_player, QOverload<>::of(&QMediaPlayer::metaDataChanged), this,
            &PlayerView::metaDataChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &PlayerView::statusChanged);
    connect(m_player, &QMediaPlayer::bufferProgressChanged, this, &PlayerView::bufferingProgress);
    connect(m_player, &QMediaPlayer::errorChanged, this, &PlayerView::displayErrorMessage);

    m_playlistModel = new PlaylistModel(this);
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
    this->setVolumeSlider(m_audioOutput->volume());

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

    connect(m_player, &QMediaPlayer::playbackStateChanged, this, &PlayerView::setPlaybackState);
    connect(m_audioOutput, &QAudioOutput::volumeChanged, this, &PlayerView::setVolumeSlider);
    //connect(m_audioOutput, &QAudioOutput::mutedChanged, controls, &PlayerControls::setMuted);

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
    return m_player->isAvailable();
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
        //if (m_playlistView)
        //    m_playlistView->setCurrentIndex(index);
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
    auto metaData = m_player->metaData();

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

    // Set kHz
    QString khz = metaData.value(QMediaMetaData::AudioCodec).toString();
    ui->khzValueLabel->setText(khz);
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
            name = QString("%1 - [%2]").arg(title).arg(QLocale::languageToString(lang));
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
    }
}

void PlayerView::playlistPositionChanged(int currentItem)
{
    //if (m_playlistView)
    //    m_playlistView->setCurrentIndex(m_playlistModel->index(currentItem, 0));

    m_player->setSource(m_playlist->currentMedia());

    if (shouldBePlaying) {
        m_player->play();
    }
}

void PlayerView::seek(int mseconds)
{
    m_player->setPosition(mseconds);
}

void PlayerView::statusChanged(QMediaPlayer::MediaStatus status)
{
    handleCursor(status);

    // handle status message
    switch (status) {
    case QMediaPlayer::NoMedia:
    case QMediaPlayer::LoadedMedia:
        setStatusInfo(QString());
        break;
    case QMediaPlayer::LoadingMedia:
        setStatusInfo(tr("Loading..."));
        break;
    case QMediaPlayer::BufferingMedia:
    case QMediaPlayer::BufferedMedia:
        setStatusInfo(tr("Buffering %1%").arg(qRound(m_player->bufferProgress() * 100.)));
        break;
    case QMediaPlayer::StalledMedia:
        setStatusInfo(tr("Stalled %1%").arg(qRound(m_player->bufferProgress() * 100.)));
        break;
    case QMediaPlayer::EndOfMedia:
        QApplication::alert(this);
        m_playlist->next();
        break;
    case QMediaPlayer::InvalidMedia:
        displayErrorMessage();
        break;
    }
}

void PlayerView::handleCursor(QMediaPlayer::MediaStatus status)
{
#ifndef QT_NO_CURSOR
    if (status == QMediaPlayer::LoadingMedia || status == QMediaPlayer::BufferingMedia
        || status == QMediaPlayer::StalledMedia)
        setCursor(QCursor(Qt::BusyCursor));
    else
        unsetCursor();
#endif
}

void PlayerView::bufferingProgress(float progress)
{
    if (m_player->mediaStatus() == QMediaPlayer::StalledMedia)
        setStatusInfo(tr("Stalled %1%").arg(qRound(progress * 100.)));
    else
        setStatusInfo(tr("Buffering %1%").arg(qRound(progress * 100.)));
}

void PlayerView::setTrackInfo(const QString &info)
{
    m_trackInfo = info;

    ui->songInfoLabel->setText(info);

    /*if (m_statusBar) {
        m_statusBar->showMessage(m_trackInfo);
        m_statusLabel->setText(m_statusInfo);
    } else {*/
    if (!m_statusInfo.isEmpty())
        setWindowTitle(QString("%1 | %2").arg(m_trackInfo).arg(m_statusInfo));
    else
        setWindowTitle(m_trackInfo);
    //}
}

void PlayerView::setStatusInfo(const QString &info)
{
    m_statusInfo = info;

    /*if (m_statusBar) {
        m_statusBar->showMessage(m_trackInfo);
        m_statusLabel->setText(m_statusInfo);
    } else {*/
    if (!m_statusInfo.isEmpty())
        setWindowTitle(QString("%1 | %2").arg(m_trackInfo).arg(m_statusInfo));
    else
        setWindowTitle(m_trackInfo);
    //}
}

void PlayerView::displayErrorMessage()
{
    if (m_player->error() == QMediaPlayer::NoError)
        return;
    setStatusInfo(m_player->errorString());
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

    m_audioOutput->setVolume(linearVolume);
}

void PlayerView::setPlaybackState(QMediaPlayer::PlaybackState state)
{
    QString imageSrc;
    switch(state) {
    case QMediaPlayer::StoppedState:
        imageSrc = ":/assets/status_stopped.png";
        break;
    case QMediaPlayer::PlayingState:
        imageSrc = ":/assets/status_playing.png";
        break;
    case QMediaPlayer::PausedState:
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

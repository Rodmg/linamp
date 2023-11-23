#include "playerview.h"
#include "ui_playerview.h"

#include "playlistmodel.h"
#include "qmediaplaylist.h"
#include "scale.h"
#include "util.h"

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

    m_system_audio = new SystemAudioControl(this);

    // Setup UI
    ui->setupUi(this);
    scale();

    //! [create-objs]
    m_player = new MediaPlayer(this);
    spectrum = new SpectrumWidget(this);
    //! [create-objs]
    connect(m_player, &MediaPlayer::durationChanged, this, &PlayerView::durationChanged);
    connect(m_player, &MediaPlayer::positionChanged, this, &PlayerView::positionChanged);
    connect(m_player, QOverload<>::of(&MediaPlayer::metaDataChanged), this,
            &PlayerView::metaDataChanged);
    connect(m_player, &MediaPlayer::mediaStatusChanged, this, &PlayerView::statusChanged);
    connect(m_player, &MediaPlayer::bufferProgressChanged, this, &PlayerView::bufferingProgress);
    connect(m_player, &MediaPlayer::errorChanged, this, &PlayerView::displayErrorMessage);

    m_playlistModel = playlistModel;
    m_playlist = m_playlistModel->playlist();
    //! [2]
    connect(m_playlist, &QMediaPlaylist::currentIndexChanged, this,
            &PlayerView::playlistPositionChanged);
    connect(m_playlist, &QMediaPlaylist::mediaAboutToBeRemoved, this,
            &PlayerView::playlistMediaRemoved);

    // duration slider and label
    ui->posBar->setRange(0, m_player->duration());
    connect(ui->posBar, &QSlider::sliderMoved, this, &PlayerView::seek);

    // Set volume slider
    ui->volumeSlider->setRange(0, 100);
    this->setVolumeSlider(m_system_audio->getVolume());
    connect(m_system_audio, &SystemAudioControl::volumeChanged, this, &PlayerView::setVolumeSlider);

    // Set Balance Slider
    ui->balanceSlider->setRange(-100, 100);
    this->setBalanceSlider(m_system_audio->getBalance());
    connect(m_system_audio, &SystemAudioControl::balanceChanged, this, &PlayerView::setBalanceSlider);

    // Reset time counter
    ui->progressTimeLabel->setText("");

    // Set play status icon
    setPlaybackState(m_player->playbackState());

    connect(ui->volumeSlider, &QSlider::valueChanged, this, &PlayerView::volumeChanged);
    connect(ui->balanceSlider, &QSlider::valueChanged, this, &PlayerView::balanceChanged);
    connect(ui->playlistButton, &QCheckBox::clicked, this, &PlayerView::showPlaylistClicked);

    connect(m_player, &MediaPlayer::playbackStateChanged, this, &PlayerView::setPlaybackState);
    //connect(m_player, &MediaPlayer::volumeChanged, this, &PlayerView::setVolumeSlider);

    // Setup spectrum widget
    connect(m_player, &MediaPlayer::newData, this, &PlayerView::handleSpectrumData);
    QVBoxLayout *spectrumLayout = new QVBoxLayout;
    spectrumLayout->addWidget(spectrum);
    spectrumLayout->setContentsMargins(0, 0, 0, 0);
    spectrumLayout->setSpacing(0);
    ui->spectrumContainer->setLayout(spectrumLayout);


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

void PlayerView::scale()
{
    this->setMaximumSize(this->maximumSize() * UI_SCALE);
    this->setMinimumSize(this->minimumSize() * UI_SCALE);

    ui->posBarContainer->layout()->setContentsMargins(ui->posBarContainer->layout()->contentsMargins() * UI_SCALE);

    ui->posBar->setMaximumHeight(ui->posBar->maximumHeight() * UI_SCALE);
    ui->posBar->setMinimumHeight(ui->posBar->minimumHeight() * UI_SCALE);
    ui->posBar->setStyleSheet(getStylesheet("playerview.posBar"));

    ui->infoContainer->setContentsMargins(ui->infoContainer->contentsMargins() * UI_SCALE);
    ui->visualizationContainer->setContentsMargins(ui->visualizationContainer->contentsMargins() * UI_SCALE);

    ui->codecDetailsContainer->layout()->setContentsMargins(ui->codecDetailsContainer->layout()->contentsMargins() * UI_SCALE);
    ui->codecDetailsContainer->layout()->setSpacing(ui->codecDetailsContainer->layout()->spacing() * UI_SCALE);
    ui->codecDetailsContainer->setStyleSheet(getStylesheet("playerview.codecDetailsContainer"));


    ui->kHzLabel->setMaximumHeight(ui->kHzLabel->maximumHeight() * UI_SCALE);
    ui->kHzLabel->setMinimumHeight(ui->kHzLabel->minimumHeight() * UI_SCALE);

    ui->kbpsLabel->setMaximumHeight(ui->kbpsLabel->maximumHeight() * UI_SCALE);
    ui->kbpsLabel->setMinimumHeight(ui->kbpsLabel->minimumHeight() * UI_SCALE);

    ui->kbpsFrame->setMinimumSize(ui->kbpsFrame->minimumSize() * UI_SCALE);
    ui->kbpsFrame->setMaximumHeight(ui->kbpsFrame->maximumHeight() * UI_SCALE);
    ui->kbpsFrame->layout()->setContentsMargins(ui->kbpsFrame->layout()->contentsMargins() * UI_SCALE);
    ui->kbpsFrame->layout()->setSpacing(ui->kbpsFrame->layout()->spacing() * UI_SCALE);
    ui->kbpsFrame->setStyleSheet(getStylesheet("playerview.kbpsFrame"));

    ui->khzFrame->setMinimumSize(ui->khzFrame->minimumSize() * UI_SCALE);
    ui->khzFrame->setMaximumHeight(ui->khzFrame->maximumHeight() * UI_SCALE);
    ui->khzFrame->layout()->setContentsMargins(ui->khzFrame->layout()->contentsMargins() * UI_SCALE);
    ui->khzFrame->layout()->setSpacing(ui->khzFrame->layout()->spacing() * UI_SCALE);
    ui->khzFrame->setStyleSheet(getStylesheet("playerview.khzFrame"));

    ui->monoLabel->setMinimumHeight(ui->monoLabel->minimumHeight() * UI_SCALE);
    ui->stereoLabel->setMinimumHeight(ui->stereoLabel->minimumHeight() * UI_SCALE);

    // Volume and balance sliders and buttons container
    ui->horizontalWidget_2->setMinimumHeight(ui->horizontalWidget_2->minimumHeight() * UI_SCALE);
    ui->horizontalWidget_2->setMaximumHeight(ui->horizontalWidget_2->maximumHeight() * UI_SCALE);
    ui->horizontalWidget_2->layout()->setContentsMargins(ui->horizontalWidget_2->layout()->contentsMargins() * UI_SCALE);

    ui->eqButton->setMinimumSize(ui->eqButton->minimumSize() * UI_SCALE);
    ui->eqButton->setMaximumSize(ui->eqButton->maximumSize() * UI_SCALE);
    ui->eqButton->setStyleSheet(getStylesheet("playerview.eqButton"));

    ui->playlistButton->setMinimumSize(ui->playlistButton->minimumSize() * UI_SCALE);
    ui->playlistButton->setMaximumSize(ui->playlistButton->maximumSize() * UI_SCALE);
    ui->playlistButton->setStyleSheet(getStylesheet("playerview.playlistButton"));

    ui->balanceSlider->setMinimumSize(ui->balanceSlider->minimumSize() * UI_SCALE);
    ui->balanceSlider->setStyleSheet(getStylesheet("playerview.balanceSlider"));

    ui->volumeSlider->setMinimumSize(ui->volumeSlider->minimumSize() * UI_SCALE);
    ui->volumeSlider->setMaximumSize(ui->volumeSlider->maximumSize() * UI_SCALE);
    ui->volumeSlider->setStyleSheet(getStylesheet("playerview.volumeSlider"));

    ui->songInfoContainer->setMinimumHeight(ui->songInfoContainer->minimumHeight() * UI_SCALE);
    ui->songInfoContainer->setMaximumHeight(ui->songInfoContainer->maximumHeight() * UI_SCALE);
    ui->songInfoContainer->layout()->setContentsMargins(ui->songInfoContainer->layout()->contentsMargins() * UI_SCALE);
    ui->songInfoContainer->setStyleSheet(getStylesheet("playerview.songInfoContainer"));

    ui->visualizationFrame->setMaximumSize(ui->visualizationFrame->maximumSize() * UI_SCALE);
    ui->visualizationFrame->setMinimumSize(ui->visualizationFrame->minimumSize() * UI_SCALE);
    ui->visualizationFrame->setStyleSheet(getStylesheet("playerview.visualizationFrame"));

    ui->playStatusIcon->setMaximumSize(ui->playStatusIcon->maximumSize() * UI_SCALE);
    ui->playStatusIcon->setMinimumSize(ui->playStatusIcon->minimumSize() * UI_SCALE);
    QRect psiGeo = ui->playStatusIcon->geometry();
    ui->playStatusIcon->setGeometry(psiGeo.x()*UI_SCALE, psiGeo.y()*UI_SCALE, psiGeo.width(), psiGeo.height());

    ui->progressTimeLabel->setGeometry(39*UI_SCALE, 3*UI_SCALE, 50*UI_SCALE, 20*UI_SCALE);
    QFont ptlFont = ui->progressTimeLabel->font();
    //ptlFont.setLetterSpacing(ptlFont.PercentageSpacing, 150); // NOT WORKING TODO
    ptlFont.setWordSpacing(-2);
    ui->progressTimeLabel->setFont(ptlFont);
    ui->progressTimeLabel->ensurePolished();

    QRect scGeo = ui->spectrumContainer->geometry();
    ui->spectrumContainer->setGeometry(scGeo.x()*UI_SCALE, scGeo.y()*UI_SCALE, scGeo.width()*UI_SCALE, scGeo.height()*UI_SCALE);
}

bool PlayerView::isPlayerAvailable() const
{
    return true; //m_player->isAvailable();
}

void PlayerView::open()
{

    QList<QUrl> urls;
    urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first())
         << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).first())
         << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::MusicLocation).first());


    QFileDialog fileDialog(this);
    QString filters = audioFileFilters().join(" ");
    fileDialog.setNameFilter("Audio (" + filters + ")");
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setFileMode(QFileDialog::ExistingFiles);
    fileDialog.setWindowTitle(tr("Open Files"));
    fileDialog.setDirectory(QStandardPaths::standardLocations(QStandardPaths::MusicLocation)
                                .value(0, QDir::homePath()));
    fileDialog.setOption(QFileDialog::ReadOnly, true);
    fileDialog.setOption(QFileDialog::DontUseNativeDialog, true);
    fileDialog.setViewMode(QFileDialog::Detail);
    fileDialog.setSidebarUrls(urls);

    #ifdef IS_EMBEDDED
    fileDialog.setWindowState(Qt::WindowFullScreen);
    #endif

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
        // Start playing only if not already playing
        if(m_player->playbackState() == MediaPlayer::PlaybackState::StoppedState) {
            auto index = m_playlistModel->index(previousMediaCount, 0);
            jump(index);
        }
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
    qint64 ms = metaData.value(QMediaMetaData::Duration).toLongLong();
    qint64 duration = ms/1000;
    QTime totalTime((duration / 3600) % 60, (duration / 60) % 60, duration % 60,
                    (duration * 1000) % 1000);
    QString durationStr = formatDuration(ms);

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
    int khz = metaData.value(QMediaMetaData::AudioCodec).toInt()/1000;
    ui->khzValueLabel->setText(khz > 0 ? QString::number(khz) : "");
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
        handlePrevious();
    } else {
        m_player->setPosition(0);
    }
}

void PlayerView::nextClicked()
{
    handleNext();
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

void PlayerView::repeatButtonClicked(bool checked)
{
    repeatEnabled = checked;
    QMediaPlaylist::PlaybackMode mode = QMediaPlaylist::PlaybackMode::Sequential;
    if(repeatEnabled) mode = QMediaPlaylist::PlaybackMode::Loop;
    m_playlist->setPlaybackMode(mode);
}

void PlayerView::shuffleButtonClicked(bool checked)
{
    shuffleEnabled = checked;
    m_playlist->setShuffle(shuffleEnabled);
}

void PlayerView::jump(const QModelIndex &index)
{
    if (index.isValid()) {
        m_playlist->setCurrentIndex(index.row());
        shouldBePlaying = true;
        m_player->play();
    }
}

void PlayerView::playlistPositionChanged(int)
{
    m_player->setSource(m_playlist->currentQueueMedia());

    if (shouldBePlaying) {
        m_player->play();
    }
}

void PlayerView::playlistMediaRemoved(int from, int to)
{
    // Stop only if currently playing file was removed
    int playingIndex = m_playlist->currentIndex();
    if(playingIndex >= from && playingIndex <= to) {
        shouldBePlaying = false;
        m_player->stop();
        m_player->clearSource();
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
        handleNext();
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

void PlayerView::handleSpectrumData(const QByteArray& data)
{
    spectrum->setData(data, m_player->format());
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
    if (m_player->error() == MediaPlayer::NoError)
        return;
    setStatusInfo(m_player->errorString());
    qDebug() << m_player->errorString();
}

void PlayerView::updateDurationInfo(qint64 currentInfo)
{
    QString tStr, tDisplayStr;
    if (currentInfo || m_duration) {
        tStr = formatDuration(currentInfo) + " / " + formatDuration(m_duration);

        QTime currentTime((currentInfo / 3600) % 60, (currentInfo / 60) % 60, currentInfo % 60,
                          (currentInfo * 1000) % 1000);
        tDisplayStr = currentTime.toString("m ss");
    }
    ui->progressTimeLabel->setText(tDisplayStr);
}

void PlayerView::setVolumeSlider(int volume)
{
    ui->volumeSlider->setValue(volume);
}

void PlayerView::setBalanceSlider(int balance)
{
    ui->balanceSlider->setValue(balance);
}

void PlayerView::volumeChanged()
{
    m_system_audio->setVolume(ui->volumeSlider->value());
}

void PlayerView::balanceChanged()
{
    // Snap slider to the center if it falls near to it
    int val = ui->balanceSlider->value();
    if(val > -20 && val < 20) {
        val = 0;
        ui->balanceSlider->setValue(val);
    }
    m_system_audio->setBalance(val);
}

void PlayerView::handlePrevious()
{
    // If is first item in playlist, do nothing
    // That's handled by QMediaPlaylist
    m_playlist->previous();
}

void PlayerView::handleNext()
{
    // If is last item in playlist:
    // If repeat enabled, go to first
    // If not, do nothing
    // That's handled by QMediaPlaylist
    m_playlist->next();
}

void PlayerView::setPlaybackState(MediaPlayer::PlaybackState state)
{
    QString imageSrc;
    switch(state) {
    case MediaPlayer::StoppedState:
        if(spectrum) spectrum->stop();
        imageSrc = ":/assets/status_stopped.png";
        break;
    case MediaPlayer::PlayingState:
        if(spectrum) spectrum->play();
        imageSrc = ":/assets/status_playing.png";
        break;
    case MediaPlayer::PausedState:
        if(spectrum) spectrum->pause();
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

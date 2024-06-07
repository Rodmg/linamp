#include "playerview.h"
#include "ui_playerview.h"

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

PlayerView::PlayerView(QWidget *parent, ControlButtonsWidget *ctlBtns) :
    QWidget(parent),
    ui(new Ui::PlayerView)
{
    // Load custom fonts
    QFontDatabase::addApplicationFont(":/assets/bignumbers.ttf");

    // Setup UI
    ui->setupUi(this);
    scale();

    controlButtons = ctlBtns;

    connect(controlButtons, &ControlButtonsWidget::playClicked, this, &PlayerView::playClicked);
    connect(controlButtons, &ControlButtonsWidget::pauseClicked, this, &PlayerView::pauseClicked);
    connect(controlButtons, &ControlButtonsWidget::stopClicked, this, &PlayerView::stopClicked);
    connect(controlButtons, &ControlButtonsWidget::nextClicked, this, &PlayerView::nextClicked);
    connect(controlButtons, &ControlButtonsWidget::previousClicked, this, &PlayerView::previousClicked);
    connect(controlButtons, &ControlButtonsWidget::openClicked, this,  &PlayerView::openClicked);
    connect(controlButtons, &ControlButtonsWidget::repeatClicked, this, &PlayerView::repeatClicked);
    connect(controlButtons, &ControlButtonsWidget::shuffleClicked, this, &PlayerView::shuffleClicked);

    // Setup spectrum analyzer
    spectrum = new SpectrumWidget(this);

    // duration slider and label
    ui->posBar->setRange(0, 0);
    connect(ui->posBar, &QSlider::sliderMoved, this, &PlayerView::positionChanged);

    // Set volume slider
    ui->volumeSlider->setRange(0, 100);
    connect(ui->volumeSlider, &QSlider::valueChanged, this, &PlayerView::volumeChanged);

    // Set Balance Slider
    ui->balanceSlider->setRange(-100, 100);
    connect(ui->balanceSlider, &QSlider::valueChanged, this, &PlayerView::handleBalanceChanged);

    // Reset time counter
    ui->progressTimeLabel->setText("");

    // Set play status icon
    setPlaybackState(MediaPlayer::StoppedState);

    connect(ui->playlistButton, &QCheckBox::clicked, this, &PlayerView::plClicked);

    // Setup spectrum widget
    QVBoxLayout *spectrumLayout = new QVBoxLayout;
    spectrumLayout->addWidget(spectrum);
    spectrumLayout->setContentsMargins(0, 0, 0, 0);
    spectrumLayout->setSpacing(0);
    ui->spectrumContainer->setLayout(spectrumLayout);

    // Setup message functionality
    messageTimer = new QTimer(this);
    connect(messageTimer, &QTimer::timeout, this, &PlayerView::clearMessage);
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
    ptlFont.setWordSpacing(-2);
    ui->progressTimeLabel->setFont(ptlFont);
    ui->progressTimeLabel->ensurePolished();

    QRect scGeo = ui->spectrumContainer->geometry();
    ui->spectrumContainer->setGeometry(scGeo.x()*UI_SCALE, scGeo.y()*UI_SCALE, scGeo.width()*UI_SCALE, scGeo.height()*UI_SCALE);

    QRect ilGeo = ui->inputLabel->geometry();
    ui->inputLabel->setGeometry(10, 6, ilGeo.width()*UI_SCALE, ilGeo.height()*UI_SCALE);
    QFont ilFont = ui->inputLabel->font();
    ilFont.setPointSize(23);
    ilFont.setFamily("DejaVu Sans Mono");
    ui->inputLabel->setFont(ilFont);
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

void PlayerView::setPosition(qint64 progress)
{
    if (!ui->posBar->isSliderDown())
        ui->posBar->setValue(progress);

    updateDurationInfo(progress / 1000);
}

void PlayerView::setSpectrumData(const QByteArray& data, QAudioFormat format)
{
    spectrum->setData(data, format);
}

void PlayerView::setMetadata(QMediaMetaData metadata)
{
    // Generate track info string
    QString artist = metadata.value(QMediaMetaData::AlbumArtist).toString().toUpper();
    QString album = metadata.value(QMediaMetaData::AlbumTitle).toString().toUpper();
    QString title = metadata.value(QMediaMetaData::Title).toString().toUpper();

    //  Calculate duration
    qint64 ms = metadata.value(QMediaMetaData::Duration).toLongLong();
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
    int bitrate = metadata.value(QMediaMetaData::AudioBitRate).toInt()/1000;
    ui->kbpsValueLabel->setText(bitrate > 0 ? QString::number(bitrate) : "");

    // Set kHz
    int khz = metadata.value(QMediaMetaData::AudioCodec).toInt()/1000;
    ui->khzValueLabel->setText(khz > 0 ? QString::number(khz) : "");
}

void PlayerView::setDuration(qint64 duration)
{
    m_duration = duration / 1000;
    ui->posBar->setMaximum(duration);
}

void PlayerView::setVolume(int volume)
{
    ui->volumeSlider->setValue(volume);
}

void PlayerView::setBalance(int balance)
{
    ui->balanceSlider->setValue(balance);
}

void PlayerView::setEqEnabled(bool enabled)
{
    eqEnabled = enabled;
    ui->eqButton->setChecked(eqEnabled);
}

void PlayerView::setPlEnabled(bool enabled)
{
    plEnabled = enabled;
    ui->playlistButton->setChecked(plEnabled);
}

void PlayerView::setShuffleEnabled(bool enabled)
{
    shuffleEnabled = enabled;
    controlButtons->setShuffleEnabled(shuffleEnabled);
}

void PlayerView::setRepeatEnabled(bool enabled)
{
    repeatEnabled = enabled;
    controlButtons->setRepeatEnabled(repeatEnabled);
}

void PlayerView::setMessage(QString message, qint64 timeout)
{
    ui->songInfoLabel->setText(message);
    messageTimer->setSingleShot(true);
    messageTimer->start(timeout);
}

void PlayerView::clearMessage()
{
    if(ui->songInfoLabel->text() != m_trackInfo) {
        ui->songInfoLabel->setText(m_trackInfo);
    }
    if(messageTimer->isActive()) {
        messageTimer->stop();
    }
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

void PlayerView::setTrackInfo(const QString &info)
{
    m_trackInfo = info;

    // Don't override info label if message is in flight
    if(!messageTimer->isActive()) {
        ui->songInfoLabel->setText(info);
    }

    if (!m_statusInfo.isEmpty())
        setWindowTitle(QString("%1 | %2").arg(m_trackInfo, m_statusInfo));
    else
        setWindowTitle(m_trackInfo);
}

void PlayerView::handleBalanceChanged()
{
    // Snap slider to the center if it falls near to it
    int val = ui->balanceSlider->value();
    if(val > -20 && val < 20) {
        val = 0;
        ui->balanceSlider->setValue(val);
    }
    emit balanceChanged(val);
}

void PlayerView::setSourceLabel(QString label)
{
    QString fLabel = "";
    for(quint8 i = 0; i < label.length(); i++) {
        fLabel.append(label[i]);
        fLabel.append("\n");
    }
    ui->inputLabel->setText(fLabel);
}

#include "controlbuttonswidget.h"
#include "ui_controlbuttonswidget.h"
#include "scale.h"

#include <QFile>

ControlButtonsWidget::ControlButtonsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ControlButtonsWidget)
{
    ui->setupUi(this);
    scale();

    connect(ui->playButton, &QPushButton::clicked, this, &ControlButtonsWidget::playClicked);
    connect(ui->pauseButton, &QPushButton::clicked, this, &ControlButtonsWidget::pauseClicked);
    connect(ui->stopButton, &QPushButton::clicked, this, &ControlButtonsWidget::stopClicked);
    connect(ui->nextButton, &QPushButton::clicked, this, &ControlButtonsWidget::nextClicked);
    connect(ui->backButton, &QPushButton::clicked, this, &ControlButtonsWidget::previousClicked);
    connect(ui->openButton, &QPushButton::clicked, this, &ControlButtonsWidget::openClicked);
    connect(ui->repeatButton, &QCheckBox::clicked, this, &ControlButtonsWidget::repeatClicked);
    connect(ui->shuffleButton, &QCheckBox::clicked, this, &ControlButtonsWidget::shuffleClicked);
}

ControlButtonsWidget::~ControlButtonsWidget()
{
    delete ui;
}

void ControlButtonsWidget::scale()
{
    ui->backButton->setMaximumWidth(ui->backButton->maximumWidth() * UI_SCALE);
    ui->backButton->setMinimumWidth(ui->backButton->minimumWidth() * UI_SCALE);
    ui->backButton->setMaximumHeight(ui->backButton->maximumHeight() * UI_SCALE);
    ui->backButton->setMinimumHeight(ui->backButton->minimumHeight() * UI_SCALE);

    ui->playButton->setMaximumWidth(ui->playButton->maximumWidth() * UI_SCALE);
    ui->playButton->setMinimumWidth(ui->playButton->minimumWidth() * UI_SCALE);
    ui->playButton->setMaximumHeight(ui->playButton->maximumHeight() * UI_SCALE);
    ui->playButton->setMinimumHeight(ui->playButton->minimumHeight() * UI_SCALE);

    ui->pauseButton->setMaximumWidth(ui->pauseButton->maximumWidth() * UI_SCALE);
    ui->pauseButton->setMinimumWidth(ui->pauseButton->minimumWidth() * UI_SCALE);
    ui->pauseButton->setMaximumHeight(ui->pauseButton->maximumHeight() * UI_SCALE);
    ui->pauseButton->setMinimumHeight(ui->pauseButton->minimumHeight() * UI_SCALE);

    ui->stopButton->setMaximumWidth(ui->stopButton->maximumWidth() * UI_SCALE);
    ui->stopButton->setMinimumWidth(ui->stopButton->minimumWidth() * UI_SCALE);
    ui->stopButton->setMaximumHeight(ui->stopButton->maximumHeight() * UI_SCALE);
    ui->stopButton->setMinimumHeight(ui->stopButton->minimumHeight() * UI_SCALE);

    ui->nextButton->setMaximumWidth(ui->nextButton->maximumWidth() * UI_SCALE);
    ui->nextButton->setMinimumWidth(ui->nextButton->minimumWidth() * UI_SCALE);
    ui->nextButton->setMaximumHeight(ui->nextButton->maximumHeight() * UI_SCALE);
    ui->nextButton->setMinimumHeight(ui->nextButton->minimumHeight() * UI_SCALE);

    ui->openButton->setMaximumWidth(ui->openButton->maximumWidth() * UI_SCALE);
    ui->openButton->setMinimumWidth(ui->openButton->minimumWidth() * UI_SCALE);
    ui->openButton->setMaximumHeight(ui->openButton->maximumHeight() * UI_SCALE);
    ui->openButton->setMinimumHeight(ui->openButton->minimumHeight() * UI_SCALE);

    ui->shuffleButton->setMaximumWidth(ui->shuffleButton->maximumWidth() * UI_SCALE);
    ui->shuffleButton->setMinimumWidth(ui->shuffleButton->minimumWidth() * UI_SCALE);
    ui->shuffleButton->setMaximumHeight(ui->shuffleButton->maximumHeight() * UI_SCALE);
    ui->shuffleButton->setMinimumHeight(ui->shuffleButton->minimumHeight() * UI_SCALE);
    ui->shuffleButton->setStyleSheet(getStylesheet("controlbuttonswidget.shuffleButton"));

    ui->repeatButton->setMaximumWidth(ui->repeatButton->maximumWidth() * UI_SCALE);
    ui->repeatButton->setMinimumWidth(ui->repeatButton->minimumWidth() * UI_SCALE);
    ui->repeatButton->setMaximumHeight(ui->repeatButton->maximumHeight() * UI_SCALE);
    ui->repeatButton->setMinimumHeight(ui->repeatButton->minimumHeight() * UI_SCALE);
    ui->repeatButton->setStyleSheet(getStylesheet("controlbuttonswidget.repeatButton"));

    this->setMaximumWidth(this->maximumWidth() * UI_SCALE);
    this->setMinimumWidth(this->minimumWidth() * UI_SCALE);
    this->setMaximumHeight(this->maximumHeight() * UI_SCALE);
    this->setMinimumHeight(this->minimumHeight() * UI_SCALE);
}

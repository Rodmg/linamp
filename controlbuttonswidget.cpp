#include "controlbuttonswidget.h"
#include "ui_controlbuttonswidget.h"

ControlButtonsWidget::ControlButtonsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ControlButtonsWidget)
{
    ui->setupUi(this);

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

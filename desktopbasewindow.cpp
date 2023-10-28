#include "desktopbasewindow.h"
#include "ui_desktopbasewindow.h"
#include "scale.h"

DesktopBaseWindow::DesktopBaseWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DesktopBaseWindow)
{
    ui->setupUi(this);
    scale();
}

DesktopBaseWindow::~DesktopBaseWindow()
{
    delete ui;
}

void DesktopBaseWindow::scale()
{
    this->setBaseSize(this->baseSize() * UI_SCALE);
    this->layout()->setContentsMargins(this->layout()->contentsMargins() * UI_SCALE);

    ui->bodyContainer->layout()->setContentsMargins(ui->bodyContainer->layout()->contentsMargins() * UI_SCALE);
    ui->bodyOuterFrame->layout()->setContentsMargins(ui->bodyOuterFrame->layout()->contentsMargins() * UI_SCALE);

    ui->titlebarContainer->setMaximumHeight(ui->titlebarContainer->maximumHeight() * UI_SCALE);
    ui->titlebarContainer->setMinimumHeight(ui->titlebarContainer->minimumHeight() * UI_SCALE);

    this->setStyleSheet(getStylesheet("desktopbasewindow"));
}

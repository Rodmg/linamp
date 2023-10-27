#include "desktopplayerwindow.h"
#include "ui_desktopplayerwindow.h"
#include "scale.h"

DesktopPlayerWindow::DesktopPlayerWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DesktopPlayerWindow)
{
    ui->setupUi(this);
    scale();
}

DesktopPlayerWindow::~DesktopPlayerWindow()
{
    delete ui;
}

void DesktopPlayerWindow::scale()
{
    this->setBaseSize(this->baseSize() * UI_SCALE);
    this->setMinimumSize(this->minimumSize() * UI_SCALE);
    this->setMaximumSize(this->maximumSize() * UI_SCALE);
    this->layout()->setContentsMargins(this->layout()->contentsMargins() * UI_SCALE);

    ui->bodyContainer->layout()->setContentsMargins(ui->bodyContainer->layout()->contentsMargins() * UI_SCALE);
    ui->bodyOuterFrame->layout()->setContentsMargins(ui->bodyOuterFrame->layout()->contentsMargins() * UI_SCALE);
    ui->bodyInnerFrame->layout()->setContentsMargins(ui->bodyInnerFrame->layout()->contentsMargins() * UI_SCALE);

    QSize sh = ui->horizontalSpacer->sizeHint();
    QSizePolicy sp = ui->horizontalSpacer->sizePolicy();
    ui->horizontalSpacer->changeSize(sh.width()*UI_SCALE, sh.height(), sp.horizontalPolicy(), sp.verticalPolicy());

    QSize vsh = ui->verticalSpacer_2->sizeHint();
    QSizePolicy vsp = ui->verticalSpacer_2->sizePolicy();
    ui->verticalSpacer_2->changeSize(vsh.width(), vsh.height()*UI_SCALE, vsp.horizontalPolicy(), vsp.verticalPolicy());

    ui->titlebarContainer->setMaximumHeight(ui->titlebarContainer->maximumHeight() * UI_SCALE);
    ui->titlebarContainer->setMinimumHeight(ui->titlebarContainer->minimumHeight() * UI_SCALE);

    // TODO Stylesheets

}

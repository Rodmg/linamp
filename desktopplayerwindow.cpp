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

    ui->dpwBodyInnerFrame->layout()->setContentsMargins(ui->dpwBodyInnerFrame->layout()->contentsMargins() * UI_SCALE);

    QSize sh = ui->horizontalSpacer->sizeHint();
    QSizePolicy sp = ui->horizontalSpacer->sizePolicy();
    ui->horizontalSpacer->changeSize(sh.width()*UI_SCALE, sh.height(), sp.horizontalPolicy(), sp.verticalPolicy());

    QSize vsh = ui->verticalSpacer_2->sizeHint();
    QSizePolicy vsp = ui->verticalSpacer_2->sizePolicy();
    ui->verticalSpacer_2->changeSize(vsh.width(), vsh.height()*UI_SCALE, vsp.horizontalPolicy(), vsp.verticalPolicy());
}

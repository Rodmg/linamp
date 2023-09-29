#include "desktopplayerwindow.h"
#include "ui_desktopplayerwindow.h"

DesktopPlayerWindow::DesktopPlayerWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DesktopPlayerWindow)
{
    ui->setupUi(this);
}

DesktopPlayerWindow::~DesktopPlayerWindow()
{
    delete ui;
}

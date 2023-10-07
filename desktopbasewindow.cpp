#include "desktopbasewindow.h"
#include "ui_desktopbasewindow.h"

DesktopBaseWindow::DesktopBaseWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DesktopBaseWindow)
{
    ui->setupUi(this);
}

DesktopBaseWindow::~DesktopBaseWindow()
{
    delete ui;
}

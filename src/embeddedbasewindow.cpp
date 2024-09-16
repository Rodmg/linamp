#include "embeddedbasewindow.h"
#include "ui_embeddedbasewindow.h"

EmbeddedBaseWindow::EmbeddedBaseWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EmbeddedBaseWindow)
{
    ui->setupUi(this);
}

EmbeddedBaseWindow::~EmbeddedBaseWindow()
{
    delete ui;
}

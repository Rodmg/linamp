#include "mainwindow.h"
#include <QVBoxLayout>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
     : QMainWindow{parent}
{
    player = new PlayerView(this);
    player->setAttribute(Qt::WidgetAttribute::WA_StyledBackground,  true);
    setCentralWidget(player);

    this->resize(825, 348);
}

MainWindow::~MainWindow()
{

}


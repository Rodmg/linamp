#include "mainmenuview.h"
#include "ui_mainmenuview.h"

MainMenuView::MainMenuView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MainMenuView)
{
    ui->setupUi(this);

    connect(ui->backButton, &QPushButton::clicked, this, &MainMenuView::backClicked);
    connect(ui->fileSourceButton, &QPushButton::clicked, this, &MainMenuView::fileSourceClicked);
    connect(ui->btSourceButton, &QPushButton::clicked, this, &MainMenuView::btSourceClicked);
    connect(ui->spotifySourceButton, &QPushButton::clicked, this, &MainMenuView::spotifySourceClicked);
    connect(ui->shutdownButton, &QPushButton::clicked, this, &MainMenuView::shutdown);
}

MainMenuView::~MainMenuView()
{
    delete ui;
}

void MainMenuView::fileSourceClicked()
{
    emit sourceSelected(0);
}

void MainMenuView::btSourceClicked()
{
    emit sourceSelected(1);
}

void MainMenuView::spotifySourceClicked()
{
    emit sourceSelected(2);
}

void MainMenuView::shutdown()
{
    QString appPath = QCoreApplication::applicationDirPath();
    QString cmd = appPath + "/shutdown.sh";

    shutdownProcess = new QProcess(this);
    shutdownProcess->start(cmd);
}

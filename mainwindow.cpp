#include "mainwindow.h"
#include "desktopplayerwindow.h"
#include "ui_desktopplayerwindow.h"
#include <QLabel>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
     : QMainWindow{parent}
{
    // Setup playlist
    m_playlistModel = new PlaylistModel(this);
    m_playlist = m_playlistModel->playlist();

    // Setup views
    player = new PlayerView(this, m_playlistModel);
    player->setAttribute(Qt::WidgetAttribute::WA_StyledBackground,  true);

    playlist = new PlaylistView(this, m_playlistModel);
    playlist->setAttribute(Qt::WidgetAttribute::WA_StyledBackground,  true);

    controlButtons = new ControlButtonsWidget(this);

    // Connect events
    connect(player, &PlayerView::showPlaylistClicked, this, &MainWindow::showPlaylist);
    connect(playlist, &PlaylistView::showPlayerClicked, this, &MainWindow::showPlayer);
    connect(playlist, &PlaylistView::songSelected, player, &PlayerView::jump);
    connect(playlist, &PlaylistView::addButtonClicked, player, &PlayerView::open);

    connect(controlButtons, &ControlButtonsWidget::playClicked, player, &PlayerView::playClicked);
    connect(controlButtons, &ControlButtonsWidget::pauseClicked, player, &PlayerView::pauseClicked);
    connect(controlButtons, &ControlButtonsWidget::stopClicked, player, &PlayerView::stopClicked);
    connect(controlButtons, &ControlButtonsWidget::nextClicked, player, &PlayerView::nextClicked);
    connect(controlButtons, &ControlButtonsWidget::previousClicked, player, &PlayerView::previousClicked);
    connect(controlButtons, &ControlButtonsWidget::openClicked, player, &PlayerView::open);
    connect(controlButtons, &ControlButtonsWidget::repeatClicked, player, &PlayerView::repeatButtonClicked);
    connect(controlButtons, &ControlButtonsWidget::shuffleClicked, player, &PlayerView::shuffleButtonClicked);

    // Prepare player main view
    DesktopPlayerWindow *playerWindow = new DesktopPlayerWindow(this);
    playerWindow->setAttribute(Qt::WidgetAttribute::WA_StyledBackground,  true);
    QVBoxLayout *playerLayout = new QVBoxLayout;
    playerLayout->setContentsMargins(0, 0, 0, 0);
    playerLayout->addWidget(player);
    playerWindow->ui->playerViewContainer->setLayout(playerLayout);
    QVBoxLayout *buttonsLayout = new QVBoxLayout;
    buttonsLayout->setContentsMargins(0, 0, 0, 0);
    buttonsLayout->addWidget(controlButtons);
    playerWindow->ui->controlButtonsContainer->setLayout(buttonsLayout);

    // Prepare navigation stack
    viewStack = new QStackedLayout;
    viewStack->addWidget(playerWindow);
    viewStack->addWidget(playlist);

    // Final UI setup and show
    QVBoxLayout *centralLayout = new QVBoxLayout;
    centralLayout->addLayout(viewStack);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(centralLayout);

    setCentralWidget(centralWidget);

    resize(831, 351);
    this->setMaximumWidth(831);
    this->setMaximumHeight(351);
    this->setMinimumWidth(831);
    this->setMinimumHeight(351);
    //setWindowFlags(Qt::CustomizeWindowHint);
}

MainWindow::~MainWindow()
{

}

void MainWindow::showPlayer()
{
    viewStack->setCurrentIndex(0);
}

void MainWindow::showPlaylist()
{
    viewStack->setCurrentIndex(1);
}

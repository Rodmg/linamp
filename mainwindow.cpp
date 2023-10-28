#include "mainwindow.h"
#include "desktopbasewindow.h"
#include "desktopplayerwindow.h"
#include "ui_desktopbasewindow.h"
#include "ui_desktopplayerwindow.h"
#include <QLabel>
#include <QVBoxLayout>
#include "scale.h"

const unsigned int WINDOW_W = 277 * UI_SCALE;
const unsigned int WINDOW_H = 117*UI_SCALE;

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
    DesktopBaseWindow *playerWindow = new DesktopBaseWindow(this);
    playerWindow->setAttribute(Qt::WidgetAttribute::WA_StyledBackground,  true);

    DesktopPlayerWindow *playerWindowContent = new DesktopPlayerWindow(this);
    QVBoxLayout *playerLayout = new QVBoxLayout;
    playerLayout->setContentsMargins(0, 0, 0, 0);
    playerLayout->addWidget(player);
    playerWindowContent->ui->playerViewContainer->setLayout(playerLayout);
    QVBoxLayout *buttonsLayout = new QVBoxLayout;
    buttonsLayout->setContentsMargins(0, 0, 0, 0);
    buttonsLayout->addWidget(controlButtons);
    playerWindowContent->ui->controlButtonsContainer->setLayout(buttonsLayout);

    QVBoxLayout *playerContentLayout = new QVBoxLayout;
    playerContentLayout->setContentsMargins(0, 0, 0, 0);
    playerContentLayout->addWidget(playerWindowContent);
    playerWindow->ui->body->setLayout(playerContentLayout);

    // Prepare playlist view
    DesktopBaseWindow *playlistWindow = new DesktopBaseWindow(this);
    playlistWindow->setAttribute(Qt::WidgetAttribute::WA_StyledBackground,  true);
    QVBoxLayout *playlistLayout = new QVBoxLayout;
    playlistLayout->setContentsMargins(0, 0, 0, 0);
    playlistLayout->addWidget(playlist);
    playlistWindow->ui->body->setLayout(playlistLayout);

    // Prepare navigation stack
    viewStack = new QStackedLayout;
    viewStack->addWidget(playerWindow);
    viewStack->addWidget(playlistWindow);

    // Final UI setup and show
    QVBoxLayout *centralLayout = new QVBoxLayout;
    centralLayout->addLayout(viewStack);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(centralLayout);

    setCentralWidget(centralWidget);

    resize(WINDOW_W, WINDOW_H);
    this->setMaximumWidth(WINDOW_W);
    this->setMaximumHeight(WINDOW_H);
    this->setMinimumWidth(WINDOW_W);
    this->setMinimumHeight(WINDOW_H);
    setWindowFlags(Qt::CustomizeWindowHint);
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

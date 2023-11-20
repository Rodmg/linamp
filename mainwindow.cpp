#include "mainwindow.h"
#include "desktopplayerwindow.h"
#include "ui_desktopplayerwindow.h"
#include "scale.h"

#ifdef IS_EMBEDDED
#include "embeddedbasewindow.h"
#include "ui_embeddedbasewindow.h"
#else
#include "desktopbasewindow.h"
#include "ui_desktopbasewindow.h"
#endif

#include <QLabel>
#include <QVBoxLayout>
#include <QMessageBox>

#ifdef IS_EMBEDDED
const unsigned int WINDOW_W = 320 * UI_SCALE;
const unsigned int WINDOW_H = 100 * UI_SCALE;
#else
const unsigned int WINDOW_W = 277 * UI_SCALE;
const unsigned int WINDOW_H = 117 * UI_SCALE;
#endif

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
    connect(playlist, &PlaylistView::addSelectedFilesClicked, player, &PlayerView::addToPlaylist);

    connect(controlButtons, &ControlButtonsWidget::playClicked, player, &PlayerView::playClicked);
    connect(controlButtons, &ControlButtonsWidget::pauseClicked, player, &PlayerView::pauseClicked);
    connect(controlButtons, &ControlButtonsWidget::stopClicked, player, &PlayerView::stopClicked);
    connect(controlButtons, &ControlButtonsWidget::nextClicked, player, &PlayerView::nextClicked);
    connect(controlButtons, &ControlButtonsWidget::previousClicked, player, &PlayerView::previousClicked);
    connect(controlButtons, &ControlButtonsWidget::openClicked, player,  &PlayerView::showPlaylistClicked);
    connect(controlButtons, &ControlButtonsWidget::repeatClicked, player, &PlayerView::repeatButtonClicked);
    connect(controlButtons, &ControlButtonsWidget::shuffleClicked, player, &PlayerView::shuffleButtonClicked);
    connect(controlButtons, &ControlButtonsWidget::logoClicked, this, &MainWindow::showShutdownModal);

    // Prepare player main view
    #ifdef IS_EMBEDDED
    EmbeddedBaseWindow *playerWindow = new EmbeddedBaseWindow(this);
    #else
    DesktopBaseWindow *playerWindow = new DesktopBaseWindow(this);
    #endif

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

    QHBoxLayout *playerContentLayout = new QHBoxLayout;
    playerContentLayout->setContentsMargins(0, 0, 0, 0);
    playerContentLayout->addWidget(playerWindowContent);
    playerWindow->ui->body->setLayout(playerContentLayout);

    // Prepare playlist view
    #ifdef IS_EMBEDDED
    EmbeddedBaseWindow *playlistWindow = new EmbeddedBaseWindow(this);
    #else
    DesktopBaseWindow *playlistWindow = new DesktopBaseWindow(this);
    #endif

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

    #ifndef IS_EMBEDDED
    setWindowFlags(Qt::CustomizeWindowHint);
    #endif
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

void MainWindow::showShutdownModal()
{
    QMessageBox msgBox;
    msgBox.setText("Shutdown");
    msgBox.setInformativeText("Turn off Linamp?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    int ret = msgBox.exec();

    switch (ret) {
    case QMessageBox::Yes:
        shutdown();
        break;
    default:
        break;
    }
}

void MainWindow::shutdown()
{
    shutdownProcess = new QProcess(this);
    shutdownProcess->start("/usr/bin/sudo" "shutdown -r now");
}

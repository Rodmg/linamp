#include "mainwindow.h"
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

    // Connect events
    connect(player, &PlayerView::showPlaylistClicked, this, &MainWindow::showPlaylist);
    connect(playlist, &PlaylistView::showPlayerClicked, this, &MainWindow::showPlayer);
    connect(playlist, &PlaylistView::songSelected, player, &PlayerView::jump);
    connect(playlist, &PlaylistView::addButtonClicked, player, &PlayerView::open);

    // Prepare navigation stack
    viewStack = new QStackedLayout;
    viewStack->addWidget(player);
    viewStack->addWidget(playlist);

    // Final UI setup and show
    QVBoxLayout *centralLayout = new QVBoxLayout;
    centralLayout->addLayout(viewStack);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(centralLayout);

    setCentralWidget(centralWidget);

    resize(825, 348);
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

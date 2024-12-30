#include <QFileDialog>

#include "mainwindow.h"
#include "desktopplayerwindow.h"
#include "qstandardpaths.h"
#include "ui_desktopplayerwindow.h"
#include "scale.h"
#include "util.h"

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

#define PY_SSIZE_T_CLEAN
#undef slots
#include <Python.h>
#define slots Q_SLOTS

#define LINAMP_PY_MODULE "linamp"

MainWindow::MainWindow(QWidget *parent)
     : QMainWindow{parent}
{
    // Initialize Python interpreter, required by audiosourcecd and audiosourcebt
    Py_Initialize();
    PyEval_SaveThread();

    // Setup playlist
    m_playlistModel = new PlaylistModel(this);
    m_playlist = m_playlistModel->playlist();

    // Setup views
    controlButtons = new ControlButtonsWidget(this);
    player = new PlayerView(this, controlButtons);
    player->setAttribute(Qt::WidgetAttribute::WA_StyledBackground,  true);

    playlist = new PlaylistView(this, m_playlistModel);
    playlist->setAttribute(Qt::WidgetAttribute::WA_StyledBackground,  true);

    coordinator = new AudioSourceCoordinator(this, player);
    fileSource = new AudioSourceFile(this, m_playlistModel);
    btSource = new AudioSourcePython(LINAMP_PY_MODULE, "BTPlayer", this);
    cdSource = new AudioSourceCD(this);
    spotSource = new AudioSourcePython(LINAMP_PY_MODULE, "SpotifyPlayer", this);
    coordinator->addSource(fileSource, "FILE", true);
    coordinator->addSource(btSource, "BT", false);
    coordinator->addSource(cdSource, "CD", false);
    coordinator->addSource(spotSource, "SPOT", false);


    // Connect events
    connect(fileSource, &AudioSourceFile::showPlaylistRequested, this, &MainWindow::showPlaylist);
    connect(playlist, &PlaylistView::showPlayerClicked, this, &MainWindow::showPlayer);
    connect(playlist, &PlaylistView::songSelected, fileSource, &AudioSourceFile::jump);
    connect(playlist, &PlaylistView::addSelectedFilesClicked, fileSource, &AudioSourceFile::addToPlaylist);

    connect(controlButtons, &ControlButtonsWidget::logoClicked, this, &MainWindow::showMenu);

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

    // Prepare menu view
    menu = new MainMenuView(this);
    menu->setAttribute(Qt::WidgetAttribute::WA_StyledBackground,  true);
    connect(menu, &MainMenuView::backClicked, this, &MainWindow::showPlayer);
    connect(menu, &MainMenuView::sourceSelected, coordinator, &AudioSourceCoordinator::setSource);

    // Prepare navigation stack
    viewStack = new QStackedLayout;
    viewStack->addWidget(playerWindow);
    viewStack->addWidget(playlistWindow);
    viewStack->addWidget(menu);

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
    PyGILState_Ensure();
    Py_Finalize();
}

void MainWindow::showPlayer()
{
    viewStack->setCurrentIndex(0);
}

void MainWindow::showPlaylist()
{
    viewStack->setCurrentIndex(1);
}

void MainWindow::showMenu()
{
    viewStack->setCurrentIndex(2);
}

void MainWindow::showShutdownModal()
{
    QMessageBox msgBox;
    msgBox.setText("Shutdown");
    msgBox.setInformativeText("Turn off Linamp?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    msgBox.setWindowFlags(Qt::FramelessWindowHint);
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
    QString appPath = QCoreApplication::applicationDirPath();
    QString cmd = appPath + "/shutdown.sh";

    shutdownProcess = new QProcess(this);
    shutdownProcess->start(cmd);
}

// Shows a standard file picker for adding items to the playlist
// Not used currently
void MainWindow::open()
{
    QList<QUrl> urls;
    urls << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first())
         << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).first())
         << QUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::MusicLocation).first());


    QFileDialog fileDialog(this);
    QString filters = audioFileFilters().join(" ");
    fileDialog.setNameFilter("Audio (" + filters + ")");
    fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
    fileDialog.setFileMode(QFileDialog::ExistingFiles);
    fileDialog.setWindowTitle(tr("Open Files"));
    fileDialog.setDirectory(QStandardPaths::standardLocations(QStandardPaths::MusicLocation)
                                .value(0, QDir::homePath()));
    fileDialog.setOption(QFileDialog::ReadOnly, true);
    fileDialog.setOption(QFileDialog::DontUseNativeDialog, true);
    fileDialog.setViewMode(QFileDialog::Detail);
    fileDialog.setSidebarUrls(urls);

#ifdef IS_EMBEDDED
    fileDialog.setWindowState(Qt::WindowFullScreen);
#endif

    if (fileDialog.exec() == QDialog::Accepted)
        fileSource->addToPlaylist(fileDialog.selectedUrls());

}

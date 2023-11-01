#include "playlistview.h"
#include "ui_playlistview.h"
#include <QScroller>
#include <QFileIconProvider>
#include <QStandardPaths>

const QString HOME_PATH = QStandardPaths::standardLocations(QStandardPaths::MusicLocation).first();

PlaylistView::PlaylistView(QWidget *parent, PlaylistModel *playlistModel) :
    QWidget(parent),
    ui(new Ui::PlaylistView)
{
    m_playlistModel = playlistModel;
    m_playlist = m_playlistModel->playlist();

    ui->setupUi(this);
    setupPlayListUi();
    setupFileBrowserUi();
    
    connect(ui->goPlayerButton, &QPushButton::clicked, this, &PlaylistView::showPlayerClicked);
    connect(m_playlist, &QMediaPlaylist::currentIndexChanged, this, &PlaylistView::playlistPositionChanged);

    connect(ui->playList, &QAbstractItemView::clicked, this, &PlaylistView::songSelected);

    connect(ui->clearButton, &QPushButton::clicked, this, &PlaylistView::clearPlaylist);
    connect(ui->removeButton, &QPushButton::clicked, this, &PlaylistView::removeItem);

    connect(ui->fbHomeButton, &QPushButton::clicked, this, &PlaylistView::fbGoHome);
    connect(ui->fbUpButton, &QPushButton::clicked, this, &PlaylistView::fbGoUp);
    connect(ui->fbSelectButton, &QPushButton::clicked, this, &PlaylistView::fbToggleSelect);
    connect(ui->fbAddButton, &QPushButton::clicked, this, &PlaylistView::fbAdd);
    connect(ui->fileBrowserListView, &QAbstractItemView::clicked, this, &PlaylistView::fbItemClicked);

}

PlaylistView::~PlaylistView()
{
    delete ui;
}

void PlaylistView::playlistPositionChanged(int currentItem)
{
    if (ui->playList)
        ui->playList->setCurrentIndex(m_playlistModel->index(currentItem, 0));
}

void PlaylistView::clearPlaylist()
{
    m_playlist->clear();
}

void PlaylistView::removeItem()
{
    QModelIndex selection = ui->playList->selectionModel()->currentIndex();
    m_playlistModel->removeRow(selection.row());
}


void PlaylistView::setupPlayListUi()
{
    // Add touch scroll to playList
    QScrollerProperties sp;
    sp.setScrollMetric(QScrollerProperties::DragVelocitySmoothingFactor, 0.6);
    sp.setScrollMetric(QScrollerProperties::MinimumVelocity, 0.0);
    sp.setScrollMetric(QScrollerProperties::MaximumVelocity, 0.5);
    sp.setScrollMetric(QScrollerProperties::AcceleratingFlickMaximumTime, 0.4);
    sp.setScrollMetric(QScrollerProperties::AcceleratingFlickSpeedupFactor, 1.2);
    sp.setScrollMetric(QScrollerProperties::SnapPositionRatio, 0.2);
    sp.setScrollMetric(QScrollerProperties::MaximumClickThroughVelocity, 0);
    sp.setScrollMetric(QScrollerProperties::DragStartDistance, 0.001);
    sp.setScrollMetric(QScrollerProperties::MousePressEventDelay, 1.0);
    sp.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor, 0.3);
    sp.setScrollMetric(QScrollerProperties::OvershootScrollDistanceFactor, 0.1);
    QScroller* playlistViewScroller = QScroller::scroller(ui->playList);
    playlistViewScroller->grabGesture(ui->playList, QScroller::LeftMouseButtonGesture);
    playlistViewScroller->setScrollerProperties(sp);

    ui->playList->setModel(m_playlistModel);
    ui->playList->setCurrentIndex(m_playlistModel->index(m_playlist->currentIndex(), 0));
    //ui->playList->setSortingEnabled(true);
    ui->playList->setDragEnabled(true);
    ui->playList->setAcceptDrops(true);
    ui->playList->setDropIndicatorShown(true);
    ui->playList->setDragDropMode(QAbstractItemView::InternalMove);
    ui->playList->setDefaultDropAction(Qt::MoveAction);
    ui->playList->setDragDropOverwriteMode(false);
}

void PlaylistView::setupFileBrowserUi()
{
    m_fileSystemModel = new QFileSystemModel(this);
    QFileIconProvider *iconProvider = new QFileIconProvider();
    m_fileSystemModel->setIconProvider(iconProvider);

    ui->fileBrowserListView->setModel(m_fileSystemModel);
    ui->fileBrowserListView->setSelectionMode(QAbstractItemView::MultiSelection);
    fbCd(HOME_PATH);
    // TODO filter only folders and audio files
}


void PlaylistView::fbCd(QString path)
{
    ui->fileBrowserListView->clearSelection();
    m_fileSystemModel->setRootPath(path);
    ui->fileBrowserListView->setRootIndex(m_fileSystemModel->index(path));
}

void PlaylistView::fbGoHome()
{
    fbCd(HOME_PATH);
}

void PlaylistView::fbGoUp()
{
    QDir dir = QDir(m_fileSystemModel->rootDirectory());
    dir.cdUp();
    fbCd(dir.absolutePath());
}

void PlaylistView::fbAdd()
{

}

void PlaylistView::fbToggleSelect()
{
    qsizetype nSelected = ui->fileBrowserListView->selectionModel()->selectedIndexes().length();
    qsizetype total = m_fileSystemModel->rowCount(m_fileSystemModel->index(m_fileSystemModel->rootPath()));
    if(nSelected == total) {
        ui->fileBrowserListView->clearSelection();
    } else {
        QModelIndex currentIndex = m_fileSystemModel->index(m_fileSystemModel->rootPath());
        int numRows = m_fileSystemModel->rowCount(currentIndex);

        for (int row = 0; row < numRows; ++row) {
            QModelIndex childIndex = m_fileSystemModel->index(row, 0, currentIndex);
            ui->fileBrowserListView->selectionModel()->select(childIndex, QItemSelectionModel::Select);
        }
    }
}

void PlaylistView::fbItemClicked(const QModelIndex &index)
{
    if(m_fileSystemModel->isDir(index)) {

    }
}


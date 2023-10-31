#include "playlistview.h"
#include "ui_playlistview.h"
#include <QScroller>

PlaylistView::PlaylistView(QWidget *parent, PlaylistModel *playlistModel) :
    QWidget(parent),
    ui(new Ui::PlaylistView)
{
    m_playlistModel = playlistModel;
    m_playlist = m_playlistModel->playlist();

    ui->setupUi(this);
    setupPlayListUi();
    
    connect(ui->goPlayerButton, &QPushButton::clicked, this, &PlaylistView::showPlayerClicked);
    connect(m_playlist, &QMediaPlaylist::currentIndexChanged, this, &PlaylistView::playlistPositionChanged);

    ui->playList->setModel(m_playlistModel);
    ui->playList->setCurrentIndex(m_playlistModel->index(m_playlist->currentIndex(), 0));
    connect(ui->playList, &QAbstractItemView::clicked, this, &PlaylistView::songSelected);

    connect(ui->clearButton, &QPushButton::clicked, this, &PlaylistView::clearPlaylist);
    connect(ui->removeButton, &QPushButton::clicked, this, &PlaylistView::removeItem);
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
    sp.setScrollMetric(QScrollerProperties::MousePressEventDelay, 0.5);
    sp.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor, 0.3);
    sp.setScrollMetric(QScrollerProperties::OvershootScrollDistanceFactor, 0.1);
    QScroller* playlistViewScroller = QScroller::scroller(ui->playList);
    playlistViewScroller->grabGesture(ui->playList, QScroller::LeftMouseButtonGesture);
    playlistViewScroller->setScrollerProperties(sp);
}

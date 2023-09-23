#include "playlistview.h"
#include "ui_playlistview.h"

PlaylistView::PlaylistView(QWidget *parent, PlaylistModel *playlistModel) :
    QWidget(parent),
    ui(new Ui::PlaylistView)
{
    m_playlistModel = playlistModel;
    m_playlist = m_playlistModel->playlist();

    ui->setupUi(this);
    
    connect(ui->goPlayerButton, &QPushButton::clicked, this, &PlaylistView::showPlayerClicked);
    connect(ui->addButton, &QPushButton::clicked, this, &PlaylistView::addButtonClicked);
    connect(m_playlist, &QMediaPlaylist::currentIndexChanged, this, &PlaylistView::playlistPositionChanged);

    ui->playList->setModel(m_playlistModel);
    ui->playList->setCurrentIndex(m_playlistModel->index(m_playlist->currentIndex(), 0));
    connect(ui->playList, &QAbstractItemView::clicked, this, &PlaylistView::songSelected);
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

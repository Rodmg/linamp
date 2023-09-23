#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include <QWidget>
#include "qmediaplaylist.h"
#include "playlistmodel.h"

namespace Ui {
class PlaylistView;
}

class PlaylistView : public QWidget
{
    Q_OBJECT

public:
    explicit PlaylistView(QWidget *parent = nullptr, PlaylistModel *playlistModel = nullptr);
    ~PlaylistView();

private:
    Ui::PlaylistView *ui;
    QMediaPlaylist *m_playlist = nullptr;
    PlaylistModel *m_playlistModel = nullptr;

private slots:
    void playlistPositionChanged(int);

signals:
    void showPlayerClicked();
    void songSelected(const QModelIndex &index);
    void addButtonClicked();
};

#endif // PLAYLISTVIEW_H

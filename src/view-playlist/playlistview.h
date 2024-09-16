#ifndef PLAYLISTVIEW_H
#define PLAYLISTVIEW_H

#include <QWidget>
#include <QUrl>
#include <QFileSystemModel>
#include <QScroller>
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
    QFileSystemModel *m_fileSystemModel = nullptr;
    QScroller *m_playlistViewScroller = nullptr;

    void setupPlayListUi();
    void setupFileBrowserUi();

    // File browser functions
    void fbCd(QString path);

private slots:
    void playlistPositionChanged(int);
    void clearPlaylist();
    void removeItem();
    void handleSongSelected(const QModelIndex &index);
    void handleSelectionChanged(int index);

    // File browser functions
    void fbGoHome();
    void fbGoUp();
    void fbAdd();
    void fbToggleSelect();

    void fbItemClicked(const QModelIndex &index);

    void updateTotalDuration();

    void toggleEditMode();

signals:
    void showPlayerClicked();
    void songSelected(const QModelIndex &index);
    void addSelectedFilesClicked(const QList<QUrl> &urls);
};

#endif // PLAYLISTVIEW_H

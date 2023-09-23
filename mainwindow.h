#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedLayout>
#include "playerview.h"
#include "playlistview.h"
#include "qmediaplaylist.h"
#include "playlistmodel.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QStackedLayout *viewStack;

    PlayerView *player;
    PlaylistView *playlist;

public slots:
    void showPlayer();
    void showPlaylist();

private:
    QMediaPlaylist *m_playlist = nullptr;
    PlaylistModel *m_playlistModel = nullptr;

};

#endif // MAINWINDOW_H

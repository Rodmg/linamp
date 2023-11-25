#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedLayout>
#include <QProcess>
#include "audiosourcecoordinator.h"
#include "audiosourcefile.h"
#include "controlbuttonswidget.h"
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
    ControlButtonsWidget *controlButtons;
    PlaylistView *playlist;
    AudioSourceCoordinator *coordinator;
    AudioSourceFile *fileSource;

public slots:
    void showPlayer();
    void showPlaylist();
    void showShutdownModal();

private:
    QMediaPlaylist *m_playlist = nullptr;
    PlaylistModel *m_playlistModel = nullptr;
    QProcess *shutdownProcess = nullptr;
    void shutdown();

};

#endif // MAINWINDOW_H

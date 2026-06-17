#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedLayout>
#include <QProcess>
#include "audiosourcecd.h"
#ifdef LINAMP_ENABLE_CD_NATIVE
#include "audiosourcecdnative.h"
#endif
#include "audiosourcecoordinator.h"
#include "audiosourcefile.h"
#include "audiosourcepython.h"
#include "controlbuttonswidget.h"
#include "mainmenuview.h"
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
    MainMenuView *menu;
    AudioSourceCoordinator *coordinator;
    AudioSourceFile *fileSource;
    AudioSourcePython *btSource;
    AudioSourceCD *cdSource;
#ifdef LINAMP_ENABLE_CD_NATIVE
    AudioSourceCDNative *cdNativeSource;
#endif
    AudioSourcePython *spotSource;

public slots:
    void showPlayer();
    void showPlaylist();
    void showMenu();
    void showShutdownModal();
    void open();

private:
    QMediaPlaylist *m_playlist = nullptr;
    PlaylistModel *m_playlistModel = nullptr;
    QProcess *shutdownProcess = nullptr;
    void shutdown();

};

#endif // MAINWINDOW_H

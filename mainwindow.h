#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "qmediaplaylist.h"

#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QAbstractItemView;
class QLabel;
class QMediaPlayer;
class QModelIndex;
class QPushButton;
class QComboBox;
class QSlider;
class QStatusBar;
class QVideoWidget;
QT_END_NAMESPACE

class PlaylistModel;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool isPlayerAvailable() const;

    void addToPlaylist(const QList<QUrl> &urls);

public slots:
    void setVolumeSlider(float volume);

private slots:
    void open();
    void durationChanged(qint64 duration);
    void positionChanged(qint64 progress);
    void metaDataChanged();

    void previousClicked();
    void playClicked();
    void pauseClicked();
    void stopClicked();

    void seek(int mseconds);
    void jump(const QModelIndex &index);
    void playlistPositionChanged(int);

    void statusChanged(QMediaPlayer::MediaStatus status);
    void bufferingProgress(float progress);

    void displayErrorMessage();

private:
    Ui::MainWindow *ui;
    void setTrackInfo(const QString &info);
    void setStatusInfo(const QString &info);
    void setPlaybackState(QMediaPlayer::PlaybackState state);
    void handleCursor(QMediaPlayer::MediaStatus status);
    void updateDurationInfo(qint64 currentInfo);
    void volumeChanged();
    QString trackName(const QMediaMetaData &metaData, int index);

    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
    QMediaPlaylist *m_playlist = nullptr;

    PlaylistModel *m_playlistModel = nullptr;
    QString m_trackInfo;
    QString m_statusInfo;
    qint64 m_duration;

    bool shuffleEnabled = false;
    bool repeatEnabled = false;
    bool shouldBePlaying = false;
};

#endif // MAINWINDOW_H

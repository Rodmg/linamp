#ifndef PLAYERVIEW_H
#define PLAYERVIEW_H

#include "qmediaplaylist.h"
#include "playlistmodel.h"
#include "mediaplayer.h"
#include "spectrumwidget.h"
#include "systemaudiocontrol.h"

#include <QMediaMetaData>
#include <QAudioOutput>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QAbstractItemView;
class QLabel;
class QModelIndex;
class QPushButton;
class QComboBox;
class QSlider;
class QStatusBar;
QT_END_NAMESPACE

namespace Ui {
class PlayerView;
}

class PlayerView : public QWidget
{
    Q_OBJECT

public:
    explicit PlayerView(QWidget *parent = nullptr, PlaylistModel *playlistModel = nullptr);
    ~PlayerView();

public slots:
    void setVolumeSlider(int volume);
    void setBalanceSlider(int balance);
    void jump(const QModelIndex &index);
    void open();
    void addToPlaylist(const QList<QUrl> &urls);

    void previousClicked();
    void nextClicked();
    void playClicked();
    void pauseClicked();
    void stopClicked();
    void repeatButtonClicked(bool checked);
    void shuffleButtonClicked(bool checked);

private slots:
    void durationChanged(qint64 duration);
    void positionChanged(qint64 progress);
    void metaDataChanged();

    void seek(int mseconds);
    void playlistPositionChanged(int);
    void playlistMediaRemoved(int, int);

    void statusChanged(MediaPlayer::MediaStatus status);
    void bufferingProgress(float progress);
    void handleSpectrumData(const QByteArray& data);

    void displayErrorMessage();

signals:
    void showPlaylistClicked();

private:
    Ui::PlayerView *ui;
    void scale();
    SpectrumWidget *spectrum = nullptr;
    void setTrackInfo(const QString &info);
    void setStatusInfo(const QString &info);
    void setPlaybackState(MediaPlayer::PlaybackState state);
    void handleCursor(MediaPlayer::MediaStatus status);
    void updateDurationInfo(qint64 currentInfo);
    void volumeChanged();
    void balanceChanged();
    void handlePrevious();
    void handleNext();

    MediaPlayer *m_player = nullptr;
    QMediaPlaylist *m_playlist = nullptr;
    SystemAudioControl *m_system_audio = nullptr;

    PlaylistModel *m_playlistModel = nullptr;
    QString m_trackInfo;
    QString m_statusInfo;
    qint64 m_duration;

    bool shuffleEnabled = false;
    bool repeatEnabled = false;
    bool shouldBePlaying = false;
};

#endif // PLAYERVIEW_H

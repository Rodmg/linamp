// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef PLAYER_H
#define PLAYER_H

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

class Player : public QWidget
{
    Q_OBJECT

public:
    explicit Player(QWidget *parent = nullptr);
    ~Player() = default;

    bool isPlayerAvailable() const;

    void addToPlaylist(const QList<QUrl> &urls);

signals:
    void fullScreenChanged(bool fullScreen);

private slots:
    void open();
    void durationChanged(qint64 duration);
    void positionChanged(qint64 progress);
    void metaDataChanged();
    void tracksChanged();

    void previousClicked();

    void seek(int mseconds);
    void jump(const QModelIndex &index);
    void playlistPositionChanged(int);

    void statusChanged(QMediaPlayer::MediaStatus status);
    void bufferingProgress(float progress);
    void videoAvailableChanged(bool available);

    void selectAudioStream();
    void selectVideoStream();
    void selectSubtitleStream();

    void displayErrorMessage();

    void audioOutputChanged(int);

private:
    void setTrackInfo(const QString &info);
    void setStatusInfo(const QString &info);
    void handleCursor(QMediaPlayer::MediaStatus status);
    void updateDurationInfo(qint64 currentInfo);
    QString trackName(const QMediaMetaData &metaData, int index);

    QMediaPlayer *m_player = nullptr;
    QAudioOutput *m_audioOutput = nullptr;
    QMediaPlaylist *m_playlist = nullptr;
    QVideoWidget *m_videoWidget = nullptr;
    QSlider *m_slider = nullptr;
    QLabel *m_labelDuration = nullptr;
    QPushButton *m_fullScreenButton = nullptr;
    QComboBox *m_audioOutputCombo = nullptr;
    QLabel *m_statusLabel = nullptr;
    QStatusBar *m_statusBar = nullptr;

    QComboBox *m_audioTracks = nullptr;
    QComboBox *m_videoTracks = nullptr;
    QComboBox *m_subtitleTracks = nullptr;

    PlaylistModel *m_playlistModel = nullptr;
    QAbstractItemView *m_playlistView = nullptr;
    QString m_trackInfo;
    QString m_statusInfo;
    qint64 m_duration;

    QWidget *m_metaDataFields[QMediaMetaData::NumMetaData] = {};
    QLabel *m_metaDataLabels[QMediaMetaData::NumMetaData] = {};
};

#endif // PLAYER_H

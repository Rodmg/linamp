// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QMEDIAPLAYLIST_P_H
#define QMEDIAPLAYLIST_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qmediaplaylist.h"
#include "qplaylistfileparser.h"

#include <QUrl>

#include <QDebug>

#ifdef Q_MOC_RUN
#    pragma Q_MOC_EXPAND_MACROS
#endif

QT_BEGIN_NAMESPACE

class QMediaPlaylistControl;

class QMediaPlaylistPrivate
{
    Q_DECLARE_PUBLIC(QMediaPlaylist)
public:
    QMediaPlaylistPrivate();

    virtual ~QMediaPlaylistPrivate();

    void loadFailed(QMediaPlaylist::Error error, const QString &errorString);

    void loadFinished();

    bool checkFormat(const char *format) const;

    void ensureParser();

    int nextPosition(int steps) const;
    int prevPosition(int steps) const;

    int nextQueuePosition(int steps) const;
    int prevQueuePosition(int steps) const;

    QList<QUrl> playlist; // Displayed playlist
    QList<QUrl> playqueue; // Actual playing queue

    int currentPos() const;
    int currentQueuePos() const;
    void setCurrentPos(int pos);
    void setCurrentQueuePos(int pos);
    QMediaPlaylist::PlaybackMode playbackMode = QMediaPlaylist::Sequential;

    QPlaylistFileParser *parser = nullptr;
    mutable QMediaPlaylist::Error error;
    mutable QString errorString;

    QMediaPlaylist *q_ptr;

private:
    int m_currentPos = -1;
    int m_currentQueuePos = -1;
};

QT_END_NAMESPACE

#endif // QMEDIAPLAYLIST_P_H

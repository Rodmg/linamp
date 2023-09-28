// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmediaplaylist_p.h"

QT_BEGIN_NAMESPACE

QMediaPlaylistPrivate::QMediaPlaylistPrivate() : error(QMediaPlaylist::NoError) { }

QMediaPlaylistPrivate::~QMediaPlaylistPrivate()
{
    delete parser;
}

void QMediaPlaylistPrivate::loadFailed(QMediaPlaylist::Error error, const QString &errorString)
{
    this->error = error;
    this->errorString = errorString;

    emit q_ptr->loadFailed();
}

void QMediaPlaylistPrivate::loadFinished()
{
    q_ptr->addMedia(parser->playlist);

    emit q_ptr->loaded();
}

bool QMediaPlaylistPrivate::checkFormat(const char *format) const
{
    QLatin1String f(format);
    QPlaylistFileParser::FileType type =
            format ? QPlaylistFileParser::UNKNOWN : QPlaylistFileParser::M3U8;
    if (format) {
        if (f == QLatin1String("m3u") || f == QLatin1String("text/uri-list")
            || f == QLatin1String("audio/x-mpegurl") || f == QLatin1String("audio/mpegurl"))
            type = QPlaylistFileParser::M3U;
        else if (f == QLatin1String("m3u8") || f == QLatin1String("application/x-mpegURL")
                 || f == QLatin1String("application/vnd.apple.mpegurl"))
            type = QPlaylistFileParser::M3U8;
    }

    if (type == QPlaylistFileParser::UNKNOWN || type == QPlaylistFileParser::PLS) {
        error = QMediaPlaylist::FormatNotSupportedError;
        errorString = QMediaPlaylist::tr("This file format is not supported.");
        return false;
    }
    return true;
}

void QMediaPlaylistPrivate::ensureParser()
{
    if (parser)
        return;

    parser = new QPlaylistFileParser(q_ptr);
    QObject::connect(parser, &QPlaylistFileParser::finished, [this]() { loadFinished(); });
    QObject::connect(parser, &QPlaylistFileParser::error,
                     [this](QMediaPlaylist::Error err, const QString &errorMsg) {
                         loadFailed(err, errorMsg);
                     });
}

int QMediaPlaylistPrivate::currentPos() const
{
    return m_currentPos;
}

int QMediaPlaylistPrivate::currentQueuePos() const
{
    return m_currentQueuePos;
}

void QMediaPlaylistPrivate::setCurrentPos(int pos)
{
    m_currentPos = pos;
    // Sync currentPlayPos
    QUrl item = playlist.at(m_currentPos);
    m_currentQueuePos = playqueue.indexOf(item);
}

void QMediaPlaylistPrivate::setCurrentQueuePos(int pos)
{
    m_currentQueuePos = pos;
    // Sync currentPos
    QUrl item = playqueue.at(m_currentQueuePos);
    m_currentPos = playlist.indexOf(item);
}

QT_END_NAMESPACE

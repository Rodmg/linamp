// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "playlistmodel.h"
#include "qmediaplaylist.h"
#include "util.h"

#include <QFileInfo>
#include <QUrl>
#include <QMimeData>
#include <QDebug>

PlaylistModel::PlaylistModel(QObject *parent) : QAbstractItemModel(parent)
{
    m_playlist.reset(new QMediaPlaylist);
    connect(m_playlist.data(), &QMediaPlaylist::mediaAboutToBeInserted, this,
            &PlaylistModel::beginInsertItems);
    connect(m_playlist.data(), &QMediaPlaylist::mediaInserted, this,
            &PlaylistModel::endInsertItems);
    connect(m_playlist.data(), &QMediaPlaylist::mediaAboutToBeRemoved, this,
            &PlaylistModel::beginRemoveItems);
    connect(m_playlist.data(), &QMediaPlaylist::mediaRemoved, this, &PlaylistModel::endRemoveItems);
    connect(m_playlist.data(), &QMediaPlaylist::mediaChanged, this, &PlaylistModel::changeItems);
}

PlaylistModel::~PlaylistModel() = default;

int PlaylistModel::rowCount(const QModelIndex &parent) const
{
    return m_playlist && !parent.isValid() ? m_playlist->mediaCount() : 0;
}

int PlaylistModel::columnCount(const QModelIndex &parent) const
{
    return !parent.isValid() ? ColumnCount : 0;
}

bool PlaylistModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid())
        return false;

    int endRow = row + (count - 1);
    for(int i = row; i <= endRow; i++) {
        m_playlist->removeMedia(i);
    }
    return true;
}

QModelIndex PlaylistModel::index(int row, int column, const QModelIndex &parent) const
{
    return m_playlist && !parent.isValid() && row >= 0 && row < m_playlist->mediaCount()
                    && column >= 0 && column < ColumnCount
            ? createIndex(row, column)
            : QModelIndex();
}

QModelIndex PlaylistModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child);

    return QModelIndex();
}

QVariant PlaylistModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(orientation);

    if(role != Qt::DisplayRole)
        return QVariant();

    switch(section) {
    case Track:
        return "TRACK";
    case Title:
        return "TITLE";
    case Artist:
        return "ARTIST";
    case Album:
        return "ALBUM";
    case Duration:
        return "DURATION";
    }
    return QVariant();
}


QVariant PlaylistModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && role == Qt::DisplayRole) {
        QMediaMetaData meta = m_playlist->mediaMetadata(index.row());

        // If the current track is playing, add play icon
        QString playIcon = "   ";
        int currentIndex = m_playlist->currentIndex();
        if(currentIndex == index.row()) {
            playIcon = " ▶ ";
        }

        switch(index.column()) {
        case Track:
            return playIcon + meta.value(QMediaMetaData::TrackNumber).toString();
        case Title:
            return meta.value(QMediaMetaData::Title);
        case Artist:
            return meta.value(QMediaMetaData::AlbumArtist);
        case Album:
            return meta.value(QMediaMetaData::AlbumTitle);
        case Duration:
            return formatDuration(meta.value(QMediaMetaData::Duration).toLongLong());
        }
    }
    return QVariant();
}

QMediaPlaylist *PlaylistModel::playlist() const
{
    return m_playlist.data();
}

bool PlaylistModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Q_UNUSED(role);
    m_data[index] = value;
    emit dataChanged(index, index);
    return true;
}

void PlaylistModel::beginInsertItems(int start, int end)
{
    m_data.clear();
    beginInsertRows(QModelIndex(), start, end);
    qDebug() << ">>>>>beginInsertItems";
}

void PlaylistModel::endInsertItems()
{
    endInsertRows();
}

void PlaylistModel::beginRemoveItems(int start, int end)
{
    m_data.clear();
    beginRemoveRows(QModelIndex(), start, end);
    qDebug() << ">>>>>beginRemoveItems";
}

void PlaylistModel::endRemoveItems()
{
    endRemoveRows();
    emit dataChanged(index(0, 0), index(m_playlist->mediaCount(), ColumnCount));
}

void PlaylistModel::changeItems(int start, int end)
{
    m_data.clear();
    emit dataChanged(index(start, 0), index(end, ColumnCount));
    qDebug() << ">>>>>changeItems";
}

Qt::ItemFlags PlaylistModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsDropEnabled;

    return Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | QAbstractItemModel::flags(index);
}

Qt::DropActions PlaylistModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

Qt::DropActions PlaylistModel::supportedDropActions() const
{
    return Qt::MoveAction;
}



QStringList PlaylistModel::mimeTypes() const
{
    QStringList types;
    types << PlaylistModel::MimeType;
    return types;
}

bool PlaylistModel::canDropMimeData(const QMimeData *data, Qt::DropAction action,
                     int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(row);
    Q_UNUSED(column);
    Q_UNUSED(parent);

    if ( action != Qt::MoveAction || !data->hasFormat(PlaylistModel::MimeType))
        return false;

    return true;
}

QMimeData *PlaylistModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData* mimeData = new QMimeData;
    QByteArray encodedData;

    QDataStream stream(&encodedData, QIODevice::WriteOnly);

    for (const QModelIndex &index : indexes) {
        if (index.isValid()) {
            QString location = m_playlist->media(index.row()).toString();
            stream << location;
        }
    }
    mimeData->setData(PlaylistModel::MimeType, encodedData);
    return mimeData;
}

bool PlaylistModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row,
                  int column, const QModelIndex &parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    if (action == Qt::IgnoreAction)
        return true;
    else if (action  != Qt::MoveAction)
        return false;

    QByteArray encodedData = data->data(PlaylistModel::MimeType);
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    QStringList newItems;
    int rows = 0;

    while (!stream.atEnd()) {
        QString text;
        stream >> text;
        newItems << text;
        ++rows;
    }

    insertRows(row, rows, QModelIndex());
    for (const QString &text : qAsConst(newItems))
    {
        QModelIndex idx = index(row, 0, QModelIndex());
        setData(idx, text);

        QUrl url = QUrl(text);
        m_playlist->insertMedia(idx.row(), url); // TODO better way to do this to avoid cuts in playback

        row++;
    }

    return true;
}


#include "moc_playlistmodel.cpp"

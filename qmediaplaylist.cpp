// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qmediaplaylist.h"
#include "qmediaplaylist_p.h"
#include "qplaylistfileparser.h"

#include <QCoreApplication>
#include <QFile>
#include <QList>
#include <QRandomGenerator>
#include <QUrl>

// Useful for debugging:
/*
void printList(QList<QUrl> list)
{
    qDebug() << "List:";
    for(unsigned int i = 0; i < list.length(); i++) {
        qDebug() << list.at(i).toString();
    }
}
*/

QT_BEGIN_NAMESPACE

class QM3uPlaylistWriter
{
public:
    QM3uPlaylistWriter(QIODevice *device)
        : m_device(device), m_textStream(new QTextStream(m_device))
    {
    }

    ~QM3uPlaylistWriter() { delete m_textStream; }

    bool writeItem(const QUrl &item)
    {
        *m_textStream << item.toString() << Qt::endl;
        return true;
    }

private:
    QIODevice *m_device;
    QTextStream *m_textStream;
};

int QMediaPlaylistPrivate::nextPosition(int steps) const
{
    if (playlist.count() == 0)
        return -1;

    int next = currentPos() + steps;

    switch (playbackMode) {
    case QMediaPlaylist::CurrentItemOnce:
        return steps != 0 ? -1 : currentPos();
    case QMediaPlaylist::CurrentItemInLoop:
        return currentPos();
    case QMediaPlaylist::Sequential:
        if (next >= playlist.size())
            next = -1;
        break;
    case QMediaPlaylist::Loop:
        next %= playlist.count();
        break;
    }

    return next;
}

int QMediaPlaylistPrivate::prevPosition(int steps) const
{
    if (playlist.count() == 0)
        return -1;

    int next = currentPos();
    if (next < 0)
        next = playlist.size();
    next -= steps;

    switch (playbackMode) {
    case QMediaPlaylist::CurrentItemOnce:
        return steps != 0 ? -1 : currentPos();
    case QMediaPlaylist::CurrentItemInLoop:
        return currentPos();
    case QMediaPlaylist::Sequential:
        if (next < 0)
            next = -1;
        break;
    case QMediaPlaylist::Loop:
        next %= playlist.size();
        if (next < 0)
            next += playlist.size();
        break;
    }

    return next;
}

int QMediaPlaylistPrivate::nextQueuePosition(int steps) const
{
    if (playqueue.count() == 0)
        return -1;

    int next = currentQueuePos() + steps;

    switch (playbackMode) {
    case QMediaPlaylist::CurrentItemOnce:
        return steps != 0 ? -1 : currentQueuePos();
    case QMediaPlaylist::CurrentItemInLoop:
        return currentQueuePos();
    case QMediaPlaylist::Sequential:
        if (next >= playqueue.size())
            next = -1;
        break;
    case QMediaPlaylist::Loop:
        next %= playqueue.count();
        break;
    }

    return next;
}

int QMediaPlaylistPrivate::prevQueuePosition(int steps) const
{
    if (playqueue.count() == 0)
        return -1;

    int next = currentQueuePos();
    if (next < 0)
        next = playqueue.size();
    next -= steps;

    switch (playbackMode) {
    case QMediaPlaylist::CurrentItemOnce:
        return steps != 0 ? -1 : currentQueuePos();
    case QMediaPlaylist::CurrentItemInLoop:
        return currentQueuePos();
    case QMediaPlaylist::Sequential:
        if (next < 0)
            next = -1;
        break;
    case QMediaPlaylist::Loop:
        next %= playqueue.size();
        if (next < 0)
            next += playqueue.size();
        break;
    }

    return next;
}

/*!
    \class QMediaPlaylist
    \inmodule QtMultimedia
    \ingroup multimedia
    \ingroup multimedia_playback


    \brief The QMediaPlaylist class provides a list of media content to play.

    QMediaPlaylist is intended to be used with other media objects,
    like QMediaPlayer.

    QMediaPlaylist allows to access the service intrinsic playlist functionality
    if available, otherwise it provides the local memory playlist implementation.

    \snippet multimedia-snippets/media.cpp Movie playlist

    Depending on playlist source implementation, most of the playlist mutating
    operations can be asynchronous.

    QMediaPlayList currently supports M3U playlists (file extension .m3u and .m3u8).

    \sa QUrl
*/

/*!
    \enum QMediaPlaylist::PlaybackMode

    The QMediaPlaylist::PlaybackMode describes the order items in playlist are played.

    \value CurrentItemOnce    The current item is played only once.

    \value CurrentItemInLoop  The current item is played repeatedly in a loop.

    \value Sequential         Playback starts from the current and moves through each successive
   item until the last is reached and then stops. The next item is a null item when the last one is
   currently playing.

    \value Loop               Playback restarts at the first item after the last has finished
   playing.

    \value Random             Play items in random order.
*/

/*!
  Create a new playlist object with the given \a parent.
*/

QMediaPlaylist::QMediaPlaylist(QObject *parent) : QObject(parent), d_ptr(new QMediaPlaylistPrivate)
{
    Q_D(QMediaPlaylist);

    d->q_ptr = this;
}

/*!
  Destroys the playlist.
  */

QMediaPlaylist::~QMediaPlaylist()
{
    delete d_ptr;
}

/*!
  \property QMediaPlaylist::playbackMode

  This property defines the order that items in the playlist are played.

  \sa QMediaPlaylist::PlaybackMode
*/

QMediaPlaylist::PlaybackMode QMediaPlaylist::playbackMode() const
{
    return d_func()->playbackMode;
}

void QMediaPlaylist::setPlaybackMode(QMediaPlaylist::PlaybackMode mode)
{
    Q_D(QMediaPlaylist);

    if (mode == d->playbackMode)
        return;

    d->playbackMode = mode;

    emit playbackModeChanged(mode);
}

void QMediaPlaylist::setShuffle(bool shuffle)
{
    shuffleEnabled = shuffle;
    if(shuffleEnabled) {
        this->shuffle();
    } else {
        this->unshuffle();
    }
}


/*!
  Returns position of the current media content in the playlist.
*/
int QMediaPlaylist::currentIndex() const
{
    return d_func()->currentPos();
}

/*!
  Returns position of the current media content in the playqueue.
*/
int QMediaPlaylist::currentQueueIndex() const
{
    return d_func()->currentQueuePos();
}

/*!
  Returns the current media content in the playlist.
*/

QUrl QMediaPlaylist::currentMedia() const
{
    Q_D(const QMediaPlaylist);
    if (d->currentPos() < 0 || d->currentPos() >= d->playlist.size())
        return QUrl();
    return d_func()->playlist.at(d_func()->currentPos());
}

/*!
  Returns the current media content in the playqueue.
*/

QUrl QMediaPlaylist::currentQueueMedia() const
{
    Q_D(const QMediaPlaylist);
    if (d->currentQueuePos() < 0 || d->currentQueuePos() >= d->playqueue.size())
        return QUrl();
    return d_func()->playqueue.at(d_func()->currentQueuePos());
}

/*!
  Returns the index of the item, which would be current after calling next()
  \a steps times.

  Returned value depends on the size of playlist, current position
  and playback mode.

  \sa QMediaPlaylist::playbackMode(), previousIndex()
*/
int QMediaPlaylist::nextIndex(int steps) const
{
    return d_func()->nextPosition(steps);
}

/*!
  Returns the index of the item, which would be current after calling previous()
  \a steps times.

  \sa QMediaPlaylist::playbackMode(), nextIndex()
*/

int QMediaPlaylist::previousIndex(int steps) const
{
    return d_func()->prevPosition(steps);
}

/*!
  Returns the index of the item, which would be current after calling next()
  \a steps times.

  Returned value depends on the size of playqueue, current position
  and playback mode.

  \sa QMediaPlaylist::playbackMode(), previousIndex()
*/
int QMediaPlaylist::nextQueueIndex(int steps) const
{
    return d_func()->nextQueuePosition(steps);
}

/*!
  Returns the index of the item, which would be current after calling previous()
  \a steps times.

  \sa QMediaPlaylist::playbackMode(), nextIndex()
*/

int QMediaPlaylist::previousQueueIndex(int steps) const
{
    return d_func()->prevQueuePosition(steps);
}

/*!
  Returns the number of items in the playlist.

  \sa isEmpty()
  */
int QMediaPlaylist::mediaCount() const
{
    return d_func()->playlist.count();
}

/*!
  Returns true if the playlist contains no items, otherwise returns false.

  \sa mediaCount()
  */
bool QMediaPlaylist::isEmpty() const
{
    return mediaCount() == 0;
}

/*!
  Returns the media content at \a index in the playlist.
*/

QUrl QMediaPlaylist::media(int index) const
{
    Q_D(const QMediaPlaylist);
    if (index < 0 || index >= d->playlist.size())
        return QUrl();
    return d->playlist.at(index);
}

/*!
  Returns the media content at \a index in the playqueue.
*/

QUrl QMediaPlaylist::queueMedia(int index) const
{
    Q_D(const QMediaPlaylist);
    if (index < 0 || index >= d->playqueue.size())
        return QUrl();
    return d->playqueue.at(index);
}

/*!
  Append the media \a content to the playlist.

  Returns true if the operation is successful, otherwise returns false.
  */
void QMediaPlaylist::addMedia(const QUrl &content)
{
    Q_D(QMediaPlaylist);
    int pos = d->playlist.size();
    emit mediaAboutToBeInserted(pos, pos);
    d->playlist.append(content);
    // Do also playqueue
    d_func()->playqueue = d_func()->playlist;
    if(shuffleEnabled) {
        this->shuffle();
    }
    emit mediaInserted(pos, pos);
}

/*!
  Append multiple media content \a items to the playlist.

  Returns true if the operation is successful, otherwise returns false.
  */
void QMediaPlaylist::addMedia(const QList<QUrl> &items)
{
    if (!items.size())
        return;

    Q_D(QMediaPlaylist);
    int first = d->playlist.size();
    int last = first + items.size() - 1;
    emit mediaAboutToBeInserted(first, last);
    d_func()->playlist.append(items);
    // Do also playqueue
    d_func()->playqueue = QList(d_func()->playlist);
    if(shuffleEnabled) {
        this->shuffle();
    }
    emit mediaInserted(first, last);
}

/*!
  Insert the media \a content to the playlist at position \a pos.

  Returns true if the operation is successful, otherwise returns false.
*/

bool QMediaPlaylist::insertMedia(int pos, const QUrl &content)
{
    Q_D(QMediaPlaylist);
    pos = qBound(0, pos, d->playlist.size());
    emit mediaAboutToBeInserted(pos, pos);
    d->playlist.insert(pos, content);

    // Do also playqueue
    d->playqueue = QList(d->playlist);
    if(shuffleEnabled) {
        this->shuffle();
    }

    emit mediaInserted(pos, pos);
    return true;
}

/*!
  Insert multiple media content \a items to the playlist at position \a pos.

  Returns true if the operation is successful, otherwise returns false.
*/

bool QMediaPlaylist::insertMedia(int pos, const QList<QUrl> &items)
{
    if (!items.size())
        return true;

    Q_D(QMediaPlaylist);
    pos = qBound(0, pos, d->playlist.size());
    int last = pos + items.size() - 1;
    emit mediaAboutToBeInserted(pos, last);
    auto newList = d->playlist.mid(0, pos);
    newList += items;
    newList += d->playlist.mid(pos);
    d->playlist = newList;

    // Do also playqueue
    d->playqueue = QList(newList);
    if(shuffleEnabled) {
        this->shuffle();
    }

    emit mediaInserted(pos, last);
    return true;
}

/*!
  Move the item from position \a from to position \a to.

  Returns true if the operation is successful, otherwise false.

  \since 5.7
*/
bool QMediaPlaylist::moveMedia(int from, int to)
{
    Q_D(QMediaPlaylist);
    if (from < 0 || from > d->playlist.count() || to < 0 || to > d->playlist.count())
        return false;

    d->playlist.move(from, to);
    emit mediaChanged(from, to);
    return true;
}

/*!
  Remove the item from the playlist at position \a pos.

  Returns true if the operation is successful, otherwise return false.
  */
bool QMediaPlaylist::removeMedia(int pos)
{
    return removeMedia(pos, pos);
}

/*!
  Remove items in the playlist from \a start to \a end inclusive.

  Returns true if the operation is successful, otherwise return false.
  */
bool QMediaPlaylist::removeMedia(int start, int end)
{
    Q_D(QMediaPlaylist);
    if (end < start || end < 0 || start >= d->playlist.count())
        return false;
    start = qBound(0, start, d->playlist.size() - 1);
    end = qBound(0, end, d->playlist.size() - 1);

    emit mediaAboutToBeRemoved(start, end);
    d->playlist.remove(start, end - start + 1);

    // Refill playqueue
    d->playqueue = QList(d->playlist);
    if(shuffleEnabled) {
        this->shuffle();
    }

    emit mediaRemoved(start, end);
    return true;
}

/*!
  Remove all the items from the playlist.

  Returns true if the operation is successful, otherwise return false.
  */
void QMediaPlaylist::clear()
{
    Q_D(QMediaPlaylist);
    int size = d->playlist.size();
    emit mediaAboutToBeRemoved(0, size - 1);
    d->playlist.clear();
    d->playqueue.clear();
    emit mediaRemoved(0, size - 1);
}

/*!
  Load playlist from \a location. If \a format is specified, it is used,
  otherwise format is guessed from location name and data.

  New items are appended to playlist.

  QMediaPlaylist::loaded() signal is emitted if playlist was loaded successfully,
  otherwise the playlist emits loadFailed().
*/

void QMediaPlaylist::load(const QUrl &location, const char *format)
{
    Q_D(QMediaPlaylist);

    d->error = NoError;
    d->errorString.clear();

    d->ensureParser();
    d->parser->start(location, QString::fromUtf8(format));
}

/*!
  Load playlist from QIODevice \a device. If \a format is specified, it is used,
  otherwise format is guessed from device data.

  New items are appended to playlist.

  QMediaPlaylist::loaded() signal is emitted if playlist was loaded successfully,
  otherwise the playlist emits loadFailed().
*/
void QMediaPlaylist::load(QIODevice *device, const char *format)
{
    Q_D(QMediaPlaylist);

    d->error = NoError;
    d->errorString.clear();

    d->ensureParser();
    d->parser->start(device, QString::fromUtf8(format));
}

/*!
  Save playlist to \a location. If \a format is specified, it is used,
  otherwise format is guessed from location name.

  Returns true if playlist was saved successfully, otherwise returns false.
  */
bool QMediaPlaylist::save(const QUrl &location, const char *format) const
{
    Q_D(const QMediaPlaylist);

    d->error = NoError;
    d->errorString.clear();

    if (!d->checkFormat(format))
        return false;

    QFile file(location.toLocalFile());

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        d->error = AccessDeniedError;
        d->errorString = tr("The file could not be accessed.");
        return false;
    }

    return save(&file, format);
}

/*!
  Save playlist to QIODevice \a device using format \a format.

  Returns true if playlist was saved successfully, otherwise returns false.
*/
bool QMediaPlaylist::save(QIODevice *device, const char *format) const
{
    Q_D(const QMediaPlaylist);

    d->error = NoError;
    d->errorString.clear();

    if (!d->checkFormat(format))
        return false;

    QM3uPlaylistWriter writer(device);
    for (const auto &entry : d->playlist)
        writer.writeItem(entry);
    return true;
}

/*!
    Returns the last error condition.
*/
QMediaPlaylist::Error QMediaPlaylist::error() const
{
    return d_func()->error;
}

/*!
    Returns the string describing the last error condition.
*/
QString QMediaPlaylist::errorString() const
{
    return d_func()->errorString;
}

/*!
  Shuffle items in the playqueue.
*/
void QMediaPlaylist::shuffle()
{
    Q_D(QMediaPlaylist);

    if(d->playqueue.empty())
        return;

    QList<QUrl> playqueue;

    // keep the current item when shuffling
    QUrl current;
    if (d->currentQueuePos() != -1)
        current = d->playqueue.takeAt(d->currentQueuePos());

    while (!d->playqueue.isEmpty())
        playqueue.append(
                d->playqueue.takeAt(QRandomGenerator::global()->bounded(int(d->playqueue.size()))));

    if (d->currentQueuePos() != -1)
        playqueue.insert(d->currentQueuePos(), current);
    d->playqueue = playqueue;
    emit mediaChanged(0, d->playqueue.count());
}

/*!
  Restore playlist into playqueue.
*/
void QMediaPlaylist::unshuffle()
{
    Q_D(QMediaPlaylist);

    if(d->playqueue.empty())
        return;

    // keep the current item when unshuffling
    QUrl current;
    if (d->currentQueuePos() != -1)
        current = d->playqueue.takeAt(d->currentQueuePos());

    QList<QUrl> playqueue(d->playlist);

    d->playqueue = playqueue;

    int newPlayPos = d->playqueue.indexOf(current);

    d->setCurrentQueuePos(newPlayPos);

    emit mediaChanged(0, d->playqueue.count());
}


/*!
    Advance to the next media content in playlist.
*/
void QMediaPlaylist::next()
{
    Q_D(QMediaPlaylist);
    int nextPosition = d->nextQueuePosition(1);
    if(nextPosition == -1) return;
    d->setCurrentQueuePos(nextPosition);

    emit currentIndexChanged(d->currentPos());
    emit currentMediaChanged(currentMedia());
}

/*!
    Return to the previous media content in playlist.
*/
void QMediaPlaylist::previous()
{
    Q_D(QMediaPlaylist);
    int prevPosition = d->prevQueuePosition(1);
    if(prevPosition == -1) return;
    d->setCurrentQueuePos(prevPosition);

    emit currentIndexChanged(d->currentPos());
    emit currentMediaChanged(currentMedia());
}

/*!
    Activate media content from playlist at position \a playlistPosition.
*/

void QMediaPlaylist::setCurrentIndex(int playlistPosition)
{
    Q_D(QMediaPlaylist);
    if (playlistPosition < 0 || playlistPosition >= d->playlist.size())
        playlistPosition = -1;
    d->setCurrentPos(playlistPosition);

    emit currentIndexChanged(d->currentPos());
    emit currentMediaChanged(currentMedia());
}

/*!
    Activate media content from playlist at position \a playlistPosition.
*/

void QMediaPlaylist::setCurrentQueueIndex(int playlistPosition)
{
    Q_D(QMediaPlaylist);
    if (playlistPosition < 0 || playlistPosition >= d->playqueue.size())
        playlistPosition = -1;
    d->setCurrentQueuePos(playlistPosition);

    emit currentIndexChanged(d->currentPos());
    emit currentMediaChanged(currentMedia());
}

/*!
    \fn void QMediaPlaylist::mediaInserted(int start, int end)

    This signal is emitted after media has been inserted into the playlist.
    The new items are those between \a start and \a end inclusive.
 */

/*!
    \fn void QMediaPlaylist::mediaRemoved(int start, int end)

    This signal is emitted after media has been removed from the playlist.
    The removed items are those between \a start and \a end inclusive.
 */

/*!
    \fn void QMediaPlaylist::mediaChanged(int start, int end)

    This signal is emitted after media has been changed in the playlist
    between \a start and \a end positions inclusive.
 */

/*!
    \fn void QMediaPlaylist::currentIndexChanged(int position)

    Signal emitted when playlist position changed to \a position.
*/

/*!
    \fn void QMediaPlaylist::playbackModeChanged(QMediaPlaylist::PlaybackMode mode)

    Signal emitted when playback mode changed to \a mode.
*/

/*!
    \fn void QMediaPlaylist::mediaAboutToBeInserted(int start, int end)

    Signal emitted when items are to be inserted at \a start and ending at \a end.
*/

/*!
    \fn void QMediaPlaylist::mediaAboutToBeRemoved(int start, int end)

    Signal emitted when item are to be deleted at \a start and ending at \a end.
*/

/*!
    \fn void QMediaPlaylist::currentMediaChanged(const QUrl &content)

    Signal emitted when current media changes to \a content.
*/

/*!
    \property QMediaPlaylist::currentIndex
    \brief Current position.
*/

/*!
    \property QMediaPlaylist::currentMedia
    \brief Current media content.
*/

/*!
    \fn QMediaPlaylist::loaded()

    Signal emitted when playlist finished loading.
*/

/*!
    \fn QMediaPlaylist::loadFailed()

    Signal emitted if failed to load playlist.
*/

/*!
    \enum QMediaPlaylist::Error

    This enum describes the QMediaPlaylist error codes.

    \value NoError                 No errors.
    \value FormatError             Format error.
    \value FormatNotSupportedError Format not supported.
    \value NetworkError            Network error.
    \value AccessDeniedError       Access denied error.
*/

QT_END_NAMESPACE

#include "moc_qmediaplaylist.cpp"

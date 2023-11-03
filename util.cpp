#include "util.h"
#include "qdatetime.h"

#include <QRegularExpression>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>

QMediaMetaData parseMetaData(const QUrl &url)
{
    QMediaMetaData metadata;
    TagLib::FileRef f(url.toLocalFile().toLocal8Bit().data());

    if(!f.isNull() && f.tag()) {
        TagLib::Tag *tag = f.tag();

        QString title = QString::fromStdString(tag->title().toCString(true));

        // If title is not set, use the file name
        if(!title.length()) {
            title = url.fileName();
        }

        QString albumTitle = QString::fromStdString(tag->album().toCString(true));
        QString artist = QString::fromStdString(tag->artist().toCString(true));
        QString comment = QString::fromStdString(tag->comment().toCString(true));
        QString genre = QString::fromStdString(tag->genre().toCString(true));
        qint64 track = tag->track();
        qint64 year = tag->year();

        metadata = QMediaMetaData{};
        metadata.insert(QMediaMetaData::Title, title);
        metadata.insert(QMediaMetaData::AlbumTitle, albumTitle);
        metadata.insert(QMediaMetaData::AlbumArtist, artist);
        metadata.insert(QMediaMetaData::Comment, comment);
        metadata.insert(QMediaMetaData::Genre, genre);
        metadata.insert(QMediaMetaData::TrackNumber, track);
        metadata.insert(QMediaMetaData::Url, url);
        metadata.insert(QMediaMetaData::Date, year);
    }

    if(!f.isNull() && f.audioProperties()) {
        TagLib::AudioProperties *properties = f.audioProperties();

        qint64 duration = properties->lengthInMilliseconds();
        qint64 bitrate = properties->bitrate() * 1000;
        qint64 sampleRate = properties->sampleRate();

        metadata.insert(QMediaMetaData::AudioBitRate, bitrate);
        metadata.insert(QMediaMetaData::AudioCodec, sampleRate); // Using AudioCodec as sample rate for now
        metadata.insert(QMediaMetaData::Duration, duration);
    }

    return metadata;
}

QString formatDuration(qint64 ms)
{
    //  Calculate duration
    qint64 duration = ms/1000;
    QTime totalTime((duration / 3600) % 60, (duration / 60) % 60, duration % 60,
                    (duration * 1000) % 1000);
    QString format = "mm:ss";
    if (duration > 3600)
        format = "hh:mm:ss";
    QString durationStr = totalTime.toString(format);
    return durationStr;
}

QStringList audioFileFilters()
{
    QStringList filters;
    filters << "*.mp3" << "*.flac" << "*.m4a" << "*.ogg" << "*.wma" << "*.wav" << "*.m3u";
    return filters;
}

bool isAudioFile(QString path)
{
    QStringList filters = audioFileFilters();

    for(const QString &filter : filters) {
        QRegularExpression rx = QRegularExpression::fromWildcard(filter,
                                                                 Qt::CaseInsensitive,
                                                                 QRegularExpression::UnanchoredWildcardConversion);
        if(rx.match(path).hasMatch()) {
            return true;
        }
    }

    return false;
}

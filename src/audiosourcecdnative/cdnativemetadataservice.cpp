#include "cdnativemetadataservice.h"

#include <discid/discid.h>

#include <cdio/cdio.h>
#include <cdio/cdtext.h>
#include <cdio/disc.h>

#include <QEventLoop>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

namespace {
struct MbTrackInfo {
    QString title;
    QString artist;
    qint64 durationMs = 0;
};

QString artistPhraseFromCredit(const QJsonArray &credits)
{
    QString phrase;
    for (const QJsonValue &creditValue : credits) {
        const QJsonObject credit = creditValue.toObject();
        const QString joiner = credit.value("joinphrase").toString();
        QString name = credit.value("name").toString();
        if (name.isEmpty()) {
            name = credit.value("artist").toObject().value("name").toString();
        }
        phrase += name;
        phrase += joiner;
    }
    return phrase.trimmed();
}

int parseTrackNumber(const QJsonObject &trackObj)
{
    const QString numberText = trackObj.value("number").toString();
    if (!numberText.isEmpty()) {
        const QRegularExpression leadingDigits("^(\\d+)");
        const QRegularExpressionMatch match = leadingDigits.match(numberText);
        if (match.hasMatch()) {
            return match.captured(1).toInt();
        }
    }

    const int position = trackObj.value("position").toInt(0);
    return position;
}

qint64 parseTrackDuration(const QJsonObject &trackObj, const QJsonObject &recordingObj)
{
    const qint64 recordingLength = recordingObj.value("length").toInteger(0);
    if (recordingLength > 0) {
        return recordingLength;
    }

    const qint64 trackLength = trackObj.value("length").toInteger(0);
    return trackLength;
}
}

CDNativeMetadataService::CDNativeMetadataService(QString devicePath)
    : m_devicePath(std::move(devicePath))
{
}

void CDNativeMetadataService::setDevicePath(const QString &devicePath)
{
    if (!devicePath.isEmpty()) {
        m_devicePath = devicePath;
    }
}

QList<CDNativeTrack> CDNativeMetadataService::enrich(const QList<CDNativeTrack> &tracks) const
{
    QList<CDNativeTrack> merged = tracks;

    applyCdText(merged);

    const QString discId = readDiscId();
    if (!discId.isEmpty()) {
        applyMusicBrainz(merged, discId);
    }

    applyFallbacks(merged);
    return merged;
}

QString CDNativeMetadataService::readDiscId() const
{
    DiscId *disc = discid_new();
    if (disc == nullptr) {
        return QString();
    }

    const QByteArray deviceBytes = m_devicePath.toLocal8Bit();
    const int ok = discid_read_sparse(disc, deviceBytes.constData(), DISCID_FEATURE_READ);
    if (ok == 0) {
        discid_free(disc);
        return QString();
    }

    const char *discId = discid_get_id(disc);
    const QString result = discId == nullptr ? QString() : QString::fromUtf8(discId);
    discid_free(disc);
    return result;
}

void CDNativeMetadataService::applyCdText(QList<CDNativeTrack> &tracks) const
{
    if (tracks.isEmpty()) {
        return;
    }

    const QByteArray deviceBytes = m_devicePath.toLocal8Bit();
    CdIo_t *cdio = cdio_open(deviceBytes.constData(), DRIVER_UNKNOWN);
    if (cdio == nullptr) {
        return;
    }

    cdtext_t *cdtext = cdio_get_cdtext(cdio);
    if (cdtext == nullptr) {
        cdio_destroy(cdio);
        return;
    }

    const char *albumTitle = cdtext_get_const(cdtext, CDTEXT_FIELD_TITLE, 0);
    const char *albumArtist = cdtext_get_const(cdtext, CDTEXT_FIELD_PERFORMER, 0);

    for (CDNativeTrack &track : tracks) {
        const char *title = cdtext_get_const(cdtext, CDTEXT_FIELD_TITLE, static_cast<track_t>(track.trackNumber));
        const char *performer = cdtext_get_const(cdtext, CDTEXT_FIELD_PERFORMER, static_cast<track_t>(track.trackNumber));

        if (title != nullptr && *title != '\0') {
            track.title = QString::fromUtf8(title);
        }
        if (performer != nullptr && *performer != '\0') {
            track.artist = QString::fromUtf8(performer);
        }
        if (albumTitle != nullptr && *albumTitle != '\0') {
            track.album = QString::fromUtf8(albumTitle);
        }
        if (albumArtist != nullptr && *albumArtist != '\0' && track.artist.isEmpty()) {
            track.artist = QString::fromUtf8(albumArtist);
        }
    }

    cdio_destroy(cdio);
}

void CDNativeMetadataService::applyMusicBrainz(QList<CDNativeTrack> &tracks, const QString &discId) const
{
    if (tracks.isEmpty()) {
        return;
    }

    QUrl url(QString("https://musicbrainz.org/ws/2/discid/%1").arg(discId));
    QUrlQuery query;
    query.addQueryItem("inc", "artists+recordings");
    query.addQueryItem("fmt", "json");
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "linamp-native-cd/1.0");
    request.setTransferTimeout(5000);

    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.get(request);

    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    QObject::connect(&timeout, &QTimer::timeout, &loop, [&]() {
        if (reply->isRunning()) {
            reply->abort();
        }
    });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timeout.start(5500);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        reply->deleteLater();
        return;
    }

    const QByteArray body = reply->readAll();
    reply->deleteLater();

    QJsonParseError jsonError;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &jsonError);
    if (jsonError.error != QJsonParseError::NoError || !doc.isObject()) {
        return;
    }

    const QJsonObject root = doc.object();
    QJsonArray releases = root.value("releases").toArray();
    if (releases.isEmpty()) {
        releases = root.value("release-list").toArray();
    }
    if (releases.isEmpty()) {
        return;
    }

    QHash<int, MbTrackInfo> mbTracks;
    QString albumTitle;
    QString albumArtist;

    bool matched = false;
    for (const QJsonValue &releaseValue : releases) {
        const QJsonObject releaseObj = releaseValue.toObject();
        const QJsonArray mediaArray = !releaseObj.value("media").toArray().isEmpty()
            ? releaseObj.value("media").toArray()
            : releaseObj.value("medium-list").toArray();

        for (const QJsonValue &mediaValue : mediaArray) {
            const QJsonObject mediaObj = mediaValue.toObject();
            QJsonArray discsArray = mediaObj.value("discs").toArray();
            if (discsArray.isEmpty()) {
                discsArray = mediaObj.value("disc-list").toArray();
            }

            bool discMatch = false;
            for (const QJsonValue &discValue : discsArray) {
                const QJsonObject discObj = discValue.toObject();
                if (discObj.value("id").toString() == discId) {
                    discMatch = true;
                    break;
                }
            }
            if (!discMatch) {
                continue;
            }

            matched = true;
            albumTitle = releaseObj.value("title").toString();
            albumArtist = artistPhraseFromCredit(releaseObj.value("artist-credit").toArray());

            QJsonArray trackArray = !mediaObj.value("tracks").toArray().isEmpty()
                ? mediaObj.value("tracks").toArray()
                : mediaObj.value("track-list").toArray();

            for (const QJsonValue &trackValue : trackArray) {
                const QJsonObject trackObj = trackValue.toObject();
                const QJsonObject recordingObj = trackObj.value("recording").toObject();

                const int trackNumber = parseTrackNumber(trackObj);
                if (trackNumber <= 0) {
                    continue;
                }

                MbTrackInfo info;
                info.title = recordingObj.value("title").toString();
                if (info.title.isEmpty()) {
                    info.title = trackObj.value("title").toString();
                }

                info.artist = artistPhraseFromCredit(recordingObj.value("artist-credit").toArray());
                info.durationMs = parseTrackDuration(trackObj, recordingObj);
                mbTracks.insert(trackNumber, info);
            }

            break;
        }

        if (matched) {
            break;
        }
    }

    if (!matched) {
        return;
    }

    for (CDNativeTrack &track : tracks) {
        const MbTrackInfo info = mbTracks.value(track.trackNumber);

        if (!albumTitle.isEmpty()) {
            track.album = albumTitle;
        }
        if (!info.title.isEmpty()) {
            track.title = info.title;
        }

        if (!info.artist.isEmpty()) {
            track.artist = info.artist;
        } else if (!albumArtist.isEmpty()) {
            track.artist = albumArtist;
        }

        if (info.durationMs > 0) {
            track.durationMs = info.durationMs;
        }
    }
}

void CDNativeMetadataService::applyFallbacks(QList<CDNativeTrack> &tracks)
{
    for (CDNativeTrack &track : tracks) {
        if (track.artist.trimmed().isEmpty()) {
            track.artist = "Unknown";
        }
        if (track.album.trimmed().isEmpty()) {
            track.album = "Unknown";
        }
        if (track.title.trimmed().isEmpty()) {
            track.title = QString("Track %1").arg(track.trackNumber);
        }
        if (track.durationMs < 0) {
            track.durationMs = 0;
        }
    }
}

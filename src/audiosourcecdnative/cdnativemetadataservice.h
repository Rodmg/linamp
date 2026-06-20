#ifndef CDNATIVEMETADATASERVICE_H
#define CDNATIVEMETADATASERVICE_H

#include <QList>
#include <QString>

#include "cdnativetrack.h"

class CDNativeMetadataService
{
public:
    explicit CDNativeMetadataService(QString devicePath = "/dev/cdrom");

    void setDevicePath(const QString &devicePath);
    QList<CDNativeTrack> enrich(const QList<CDNativeTrack> &tracks) const;

private:
    QString m_devicePath;

    QString readDiscId() const;
    void applyCdText(QList<CDNativeTrack> &tracks) const;
    void applyMusicBrainz(QList<CDNativeTrack> &tracks, const QString &discId) const;
    static void applyFallbacks(QList<CDNativeTrack> &tracks);
};

#endif // CDNATIVEMETADATASERVICE_H

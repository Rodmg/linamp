#ifndef CDNATIVEDISCSERVICE_H
#define CDNATIVEDISCSERVICE_H

#include <QList>
#include <QString>

#include "cdnativetrack.h"

class CDNativeDiscService
{
public:
    explicit CDNativeDiscService(QString devicePath = "/dev/cdrom");

    bool isDiscInserted() const;
    bool eject() const;
    QList<CDNativeTrack> readTrackLayout() const;

private:
    QString m_devicePath;

    static qint64 msfToLba(int minute, int second, int frame);
};

#endif // CDNATIVEDISCSERVICE_H

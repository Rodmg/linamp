#ifndef CDNATIVETRACK_H
#define CDNATIVETRACK_H

#include <QString>

struct CDNativeTrack {
    int trackNumber = 0;
    qint64 startLba = 0;
    qint64 lengthLba = 0;
    qint64 durationMs = 0;
    bool isDataTrack = false;

    QString artist = "Unknown";
    QString album = "Unknown";
    QString title;
};

#endif // CDNATIVETRACK_H

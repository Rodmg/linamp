#ifndef UTIL_H
#define UTIL_H

#include <QMediaMetaData>
#include <QUrl>

QMediaMetaData parseMetaData(const QUrl &url);

QString formatDuration(qint64 ms);

#endif // UTIL_H

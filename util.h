#ifndef UTIL_H
#define UTIL_H

#include <QMediaMetaData>
#include <QUrl>

QMediaMetaData parseMetaData(const QUrl &url);

QString formatDuration(qint64 ms);

QStringList audioFileFilters();

bool isAudioFile(QString path);

#endif // UTIL_H

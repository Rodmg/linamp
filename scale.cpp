#include "scale.h"

#include <QFile>

QString getStylesheet(QString name)
{
    QFile file(":/styles/" + name + "." + QString::number(UI_SCALE) + "x.qss");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    return styleSheet;
}

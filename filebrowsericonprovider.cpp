#include "filebrowsericonprovider.h"
#include <QRegularExpression>

FileBrowserIconProvider::FileBrowserIconProvider()
{
    folderIcon = new QIcon(":/assets/fb_folderIcon.png");
    folderIcon->addFile(":/assets/fb_folderIcon_selected.png", QSize(), QIcon::Selected);

    musicIcon = new QIcon(":/assets/fb_musicIcon.png");
    musicIcon->addFile(":/assets/fb_musicIcon_selected.png", QSize(), QIcon::Selected);
}

QIcon FileBrowserIconProvider::icon(QAbstractFileIconProvider::IconType type) const
{
    if(type == QAbstractFileIconProvider::IconType::Folder) {
        return *folderIcon;
    }

    return QFileIconProvider::icon(type);
}

QIcon FileBrowserIconProvider::icon(const QFileInfo &info) const
{

    if(info.isDir()) {
        return *folderIcon;
    }

    const QString &path = info.absoluteFilePath();

    QStringList filters;
    filters << "*.mp3" << "*.flac" << "*.m4a" << "*.ogg" << "*.wma" << "*.wav" << "*.m3u";

    for(const QString &filter : filters) {
        QRegularExpression rx = QRegularExpression::fromWildcard(filter,
                                                                 Qt::CaseInsensitive,
                                                                 QRegularExpression::UnanchoredWildcardConversion);
        if(rx.match(path).hasMatch()) {
            return *musicIcon;
        }
    }

    return QFileIconProvider::icon(info);
}

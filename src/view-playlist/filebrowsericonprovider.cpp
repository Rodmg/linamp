#include "filebrowsericonprovider.h"
#include "util.h"
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

    if(isAudioFile(path)) {
        return *musicIcon;
    }

    return QFileIconProvider::icon(info);
}

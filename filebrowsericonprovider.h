#ifndef FILEBROWSERICONPROVIDER_H
#define FILEBROWSERICONPROVIDER_H

#include <QFileIconProvider>
#include <QIcon>

class FileBrowserIconProvider : public QFileIconProvider
{
public:
    FileBrowserIconProvider();

    QIcon icon(QAbstractFileIconProvider::IconType type) const override;
    QIcon icon(const QFileInfo &info) const override;

private:
    QIcon *folderIcon = nullptr;
    QIcon *musicIcon = nullptr;
};

#endif // FILEBROWSERICONPROVIDER_H

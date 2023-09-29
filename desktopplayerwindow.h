#ifndef DESKTOPPLAYERWINDOW_H
#define DESKTOPPLAYERWINDOW_H

#include <QWidget>

namespace Ui {
class DesktopPlayerWindow;
}

class DesktopPlayerWindow : public QWidget
{
    Q_OBJECT

public:
    explicit DesktopPlayerWindow(QWidget *parent = nullptr);
    ~DesktopPlayerWindow();
    Ui::DesktopPlayerWindow *ui;
};

#endif // DESKTOPPLAYERWINDOW_H

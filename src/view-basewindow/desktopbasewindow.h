#ifndef DESKTOPBASEWINDOW_H
#define DESKTOPBASEWINDOW_H

#include <QWidget>

namespace Ui {
class DesktopBaseWindow;
}

class DesktopBaseWindow : public QWidget
{
    Q_OBJECT

public:
    explicit DesktopBaseWindow(QWidget *parent = nullptr);
    ~DesktopBaseWindow();
    Ui::DesktopBaseWindow *ui;

private:
    void scale();
};

#endif // DESKTOPBASEWINDOW_H

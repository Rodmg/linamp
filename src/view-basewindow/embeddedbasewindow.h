#ifndef EMBEDDEDBASEWINDOW_H
#define EMBEDDEDBASEWINDOW_H

#include <QWidget>

namespace Ui {
class EmbeddedBaseWindow;
}

class EmbeddedBaseWindow : public QWidget
{
    Q_OBJECT

public:
    explicit EmbeddedBaseWindow(QWidget *parent = nullptr);
    ~EmbeddedBaseWindow();
    Ui::EmbeddedBaseWindow *ui;
};

#endif // EMBEDDEDBASEWINDOW_H

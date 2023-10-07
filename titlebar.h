#ifndef TITLEBAR_H
#define TITLEBAR_H

#include <QWidget>

namespace Ui {
class TitleBar;
}

class TitleBar : public QWidget
{
    Q_OBJECT

public:
    explicit TitleBar(QWidget *parent = nullptr);
    ~TitleBar();

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

private:
    Ui::TitleBar *ui;
    QPointF m_pCursor;
};

#endif // TITLEBAR_H

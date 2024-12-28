#ifndef LINAMPSLIDER_H
#define LINAMPSLIDER_H

#include <QSlider>
#include <QImage>
#include <QColor>

class LinampSlider : public QSlider
{
    Q_OBJECT
public:
    LinampSlider(QWidget* parent);

    void setGradient(QColor from, QColor to, Qt::Orientation orientation);

    void setGradient(QList<QColor> steps, Qt::Orientation orientation);

private slots:
    void changeColor(int);

private:
    QImage gradient;
    QString baseStylesheet;
};

#endif // LINAMPSLIDER_H

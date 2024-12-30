#include "linampslider.h"
#include <QLinearGradient>
#include <QPainter>

LinampSlider::LinampSlider(QWidget* parent) :
    QSlider(parent),
    gradient(QSize(100,100), QImage::Format_RGB32)
{
}

void LinampSlider::setGradient(QColor from, QColor to, Qt::Orientation orientation)
{
    setOrientation(orientation);
    // create linear gradient
    QLinearGradient linearGrad(QPointF(0, 0), (orientation==Qt::Horizontal) ? QPointF(100, 0) : QPointF(0, 100));
    linearGrad.setColorAt(0, from);
    linearGrad.setColorAt(1, to);

    // paint gradient in a QImage:
    QPainter p(&gradient);
    p.fillRect(gradient.rect(), linearGrad);

    connect(this, SIGNAL(valueChanged(int)), this, SLOT(changeColor(int)));

    // initialize
    changeColor(value());
}

void LinampSlider::setGradient(QList<QColor> steps, Qt::Orientation orientation)
{
    setOrientation(orientation);
    // create linear gradient
    QLinearGradient linearGrad(QPointF(0, 0), (orientation==Qt::Horizontal) ? QPointF(100, 0) : QPointF(0, 100));
    for(int i = 0; i < steps.count(); i++) {
        linearGrad.setColorAt(float(i)/float(steps.count() - 1), steps[i]);
    }

    // paint gradient in a QImage:
    QPainter p(&gradient);
    p.fillRect(gradient.rect(), linearGrad);

    connect(this, SIGNAL(valueChanged(int)), this, SLOT(changeColor(int)));

    // initialize
    changeColor(value());
}

void LinampSlider::changeColor(int pos)
{
    QColor color;

    if (orientation() == Qt::Horizontal)
    {
        // retrieve color index based on cursor position
        int posIndex = gradient.size().width() * (pos - minimum()) / (maximum() - minimum());
        posIndex = std::min(posIndex, gradient.width() - 1);

        // pickup appropriate color
        color = gradient.pixel(posIndex, gradient.size().height()/2);
    }
    else
    {
        // retrieve color index based on cursor position
        int posIndex = gradient.size().height() * (pos - minimum()) / (maximum() - minimum());
        posIndex = std::min(posIndex, gradient.height() - 1);

        // pickup appropriate color
        color = gradient.pixel(gradient.size().width()/2, posIndex);
    }

    // create and apply stylesheet!
    // can be customized to change background and handle border!

    if(baseStylesheet.length() == 0) {
        // Capture original unmodified stylesheet the first time
        baseStylesheet = styleSheet();
    }

    // Append background color
    QString stylesheet = baseStylesheet +
        "QSlider::sub-page:" + ((orientation() == Qt::Horizontal) ? QString("horizontal"):QString("vertical")) + "{ \
            background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1, stop: 0 " + color.darker(130).name() + ", stop: 1 " + color.name() + "); \
        }" +
        "QSlider::add-page:" + ((orientation() == Qt::Horizontal) ? QString("horizontal"):QString("vertical")) + "{ \
            background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1, stop: 0 " + color.darker(130).name() + ", stop: 1 " + color.name() + "); \
        }";

    setStyleSheet(stylesheet);
}

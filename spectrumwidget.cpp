#include "spectrumwidget.h"
#include "fft.h"
#include <QPainter>
#include <QColor>

const QColor specBarColors[16] = {
    QColor::fromRgb(192,0,0),
    QColor::fromRgb(191,7,0),
    QColor::fromRgb(191,28,0),
    QColor::fromRgb(191,59,0),
    QColor::fromRgb(191,95,0),
    QColor::fromRgb(191,132,0),
    QColor::fromRgb(191,163,0),
    QColor::fromRgb(191,183,0),
    QColor::fromRgb(191,191,0),
    QColor::fromRgb(183,191,0),
    QColor::fromRgb(163,191,0),
    QColor::fromRgb(132,191,0),
    QColor::fromRgb(95,191,0),
    QColor::fromRgb(59,191,0),
    QColor::fromRgb(28,191,0),
    QColor::fromRgb(7,191,0),
};

QLinearGradient *specBarGradient = nullptr;

QLinearGradient* getSpecBarGradient()
{
    if(!specBarGradient) {
        specBarGradient = new QLinearGradient(QPointF(0, 0), QPointF(0, 40));
        for(int i = 0; i < 16; i++) {
            specBarGradient->setColorAt(float(i)/15.0, specBarColors[i]);
        }
    }
    return specBarGradient;
}

SpectrumWidget::SpectrumWidget(QWidget *parent)
    : QWidget{parent}
{
    m_renderTimer = new QTimer(this);
    m_renderTimer->setInterval(33); // around 30 fps
    connect(m_renderTimer, &QTimer::timeout, this, QOverload<>::of(&SpectrumWidget::update));
}

void SpectrumWidget::play()
{
    m_playing = true;
    if(m_renderTimer) m_renderTimer->start();
}

void SpectrumWidget::pause()
{
    m_playing = false;
    if(m_renderTimer) m_renderTimer->stop();
}

void SpectrumWidget::stop()
{
    m_playing = false;
    if(m_renderTimer) m_renderTimer->stop();
    this->update();
}

void SpectrumWidget::paint_spectrum (QPainter & p)
{
    for (int i = 0; i < N_BANDS; i++)
    {
        // Bar measures 3px*3 wide, 1px*3 spacing
        int x = (9 * i) + 3*i;
        //auto color = QColor(7, 191, 0); //audqt::vis_bar_color(palette().color (QPalette::Highlight), i, N_BANDS);

        p.fillRect(x + 3, height() - (m_bandValues[i] * height() / 40),
                   9, (m_bandValues[i] * height() / 40), *getSpecBarGradient());
    }
}

void SpectrumWidget::paintEvent (QPaintEvent *)
{
    QPainter p(this);

    if(m_playing) {
        float freq[N / 2];
        calc_freq(m_data, freq);
        for(int i = 0; i < N_BANDS; i++) {
            m_bandValues[i] = freq[i];
        }
        paint_spectrum(p);
    }
}

void SpectrumWidget::setData(const QByteArray& data)
{
    for(int i = 0; i < DFT_SIZE; i++) {
        if(i < data.length()) {
            m_data[i] = data.at(i);
        }
    }
}

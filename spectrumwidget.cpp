#include "spectrumwidget.h"
#include "fft.h"
#include <QPainter>

SpectrumWidget::SpectrumWidget(QWidget *parent)
    : QWidget{parent}
{
    m_renderTimer = new QTimer(this);
    m_renderTimer->setInterval(33); // around 30 fps
    connect(m_renderTimer, &QTimer::timeout, this, QOverload<>::of(&SpectrumWidget::update));
}

void SpectrumWidget::play()
{
    qDebug() << "Spectrum play";
    m_playing = true;
    if(m_renderTimer) m_renderTimer->start();
}

void SpectrumWidget::pause()
{
    qDebug() << "Spectrum pause";
    m_playing = false;
    if(m_renderTimer) m_renderTimer->stop();
}

void SpectrumWidget::stop()
{
    qDebug() << "Spectrum stop";
    m_playing = false;
    if(m_renderTimer) m_renderTimer->stop();
    this->update();
}

void SpectrumWidget::paint_spectrum (QPainter & p)
{
    for (int i = 0; i < N_BANDS; i++)
    {
        int x = ((width () / N_BANDS) * i) + 2;
        auto color = QColor(100, 123, 234); //audqt::vis_bar_color(palette().color (QPalette::Highlight), i, N_BANDS);

        p.fillRect(x + 1, height() - (m_bandValues[i] * height() / 40),
                   (width() / N_BANDS) - 1, (m_bandValues[i] * height() / 40), color);
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

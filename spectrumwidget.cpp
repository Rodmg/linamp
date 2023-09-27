#include "spectrumwidget.h"
#include "fft.h"
#include <QPainter>
#include <QColor>

#define VIS_DELAY 2 /* delay before falloff in frames */
#define VIS_FALLOFF 2 /* falloff in pixels per frame */
#define VIS_PEAK_DELAY 16
#define VIS_PEAK_FALLOFF 1 /* falloff in pixels per frame */


static void pcm_to_mono(const float * data, float * mono, int channels)
{
    if (channels == 1)
        memcpy(mono, data, sizeof(float) * 512);
    else
    {
        float * set = mono;
        while (set < &mono[512])
        {
            *set++ = (data[0] + data[1]) / 2;
            data += channels;
        }
    }
}

static float compute_freq_band(const float * freq,
                               const float * xscale, int band,
                               int bands)
{
    int a = ceilf(xscale[band]);
    int b = floorf(xscale[band + 1]);
    float n = 0;

    if (b < a)
        n += freq[b] * (xscale[band + 1] - xscale[band]);
    else
    {
        if (a > 0)
            n += freq[a - 1] * (a - xscale[band]);
        for (; a < b; a++)
            n += freq[a];
        if (b < 256)
            n += freq[b] * (xscale[band + 1] - b);
    }

    /* fudge factor to make the graph have the same overall height as a
       12-band one no matter how many bands there are */
    n *= (float)bands / 12;

    return 20 * log10f(n);
}

static void compute_log_xscale(float * xscale, int bands)
{
    for (int i = 0; i <= bands; i++)
        xscale[i] = powf(256, (float)i / bands) - 0.5f;
}

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
    clear();
    compute_log_xscale(m_xscale, N_BANDS);
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
    clear();
    this->update();
}

void SpectrumWidget::paintBackground(QPainter & p)
{
    // Paint the gray pixels behind the spectrum
    // Pixels: 1px*3 x 1px*3
    // Spacing: 1px*3 for both x and y
    const unsigned int rows = 8;
    const unsigned int cols = 38;

    // Starting points
    unsigned int x = 3;
    unsigned int y = 4;

    const QColor color = QColor::fromRgb(64, 64, 64);

    for(unsigned int row = 0; row < rows; row++) {
        for(unsigned int col = 0; col < cols; col++) {
            p.fillRect(x, y, 3, 3, color);
            x += 6; // Add pixel width + spacing
        }
        x = 3;  // Go back to the start of the row
        y += 6; // Add pixel height + spacing
    }
}


void SpectrumWidget::paintSpectrum (QPainter & p)
{
    for (int i = 0; i < N_BANDS; i++) {
        // Bar measures 3px*3 wide, 1px*3 spacing
        int x = (9 * i) + 3*i;
        p.fillRect(x + 3, height() - (m_bandValues[i] * height() / 40),
                   9, (m_bandValues[i] * height() / 40), *getSpecBarGradient());
    }
}

void SpectrumWidget::paintPeaks (QPainter & p)
{
    const QColor color = QColor::fromRgb(191, 191, 191);
    for (int i = 0; i < N_BANDS; i++) {
        // Peak rectangle measures 3px*3 wide, 1px*3 high, 1px*3 spacing
        int x = (9 * i) + 3*i;
        p.fillRect(x + 3, height() - (m_peakValues[i] * height() / 40),
                   9, 3, color);
    }
}


void SpectrumWidget::paintEvent (QPaintEvent *)
{
    QPainter p(this);

    paintBackground(p);

    if(m_playing) {
        float mono[N];
        float freq[N / 2];
        int channels = 2; // TODO get from format
        pcm_to_mono(m_data, mono, channels);
        calc_freq(mono, freq);

        for(int i = 0; i < N/2; i++) {
            qDebug() << "freq " << i << ": " << freq[i];
        }

        for(int i = 0; i < N_BANDS; i ++)
        {
            /* 40 dB range */
            int x = /*40 +*/ compute_freq_band(freq, m_xscale, i, N_BANDS);
            x = std::clamp(x, 0, 40);

            m_bandValues[i] -= std::max(0, VIS_FALLOFF - m_bandDelays[i]);

            if (m_bandDelays[i])
                m_bandDelays[i]--;

            if (x > m_bandValues[i])
            {
                m_bandValues[i] = x;
                m_bandDelays[i] = VIS_DELAY;
            }

            m_peakValues[i] -= std::max(0, VIS_PEAK_FALLOFF - m_peakDelays[i]);

            if (m_peakDelays[i])
                m_peakDelays[i]--;

            if (x > m_peakValues[i])
            {
                m_peakValues[i] = x;
                m_peakDelays[i] = VIS_PEAK_DELAY;
            }
        }

        for(int i = 0; i < N_BANDS; i++) {
            qDebug() << "band " << i << ": " << m_bandValues[i];
        }

        paintSpectrum(p);
        paintPeaks(p);
    }
}

void SpectrumWidget::setData(const QByteArray& data)
{
    // TODO: this needs to vary depending on source codec...
    for(int i = 0; i < DFT_SIZE; i++) {
        if(i < data.length()) {
            m_data[i] = (float)data[i];
        }
    }
}

void SpectrumWidget::clear() {
    memset(m_data, 0, sizeof m_data);
    memset(m_bandValues, 0, sizeof m_bandValues);
    memset(m_bandDelays, 0, sizeof m_bandDelays);
}

#include "spectrumwidget.h"
#include "fft.h"
#include <QPainter>
#include <QColor>
#include "scale.h"

#define VIS_DELAY 1 /* delay before falloff in frames */
#define VIS_FALLOFF 4 /* falloff in pixels per frame */
#define VIS_PEAK_DELAY 16
#define VIS_PEAK_FALLOFF 1 /* falloff in pixels per frame */

const quint16 PCMS16MaxAmplitude = 32768; // because minimum is -32768

float pcmToFloat(qint16 pcm)
{
    return float(pcm) / PCMS16MaxAmplitude;
}

static void floatPcmToMono(const float *data, float *mono, int channels)
{
    if (channels == 1) {
        memcpy(mono, data, sizeof(float) * N);
    }
    else {
        float *set = mono;
        while (set < &mono[N]) {
            *set++ = (data[0] + data[1]) / 2;
            data += channels;
        }
    }
}

static float computeFreqBand(const float *freq,
                               const float *xscale, int band,
                               int bands)
{
    int a = ceilf(xscale[band]);
    int b = floorf(xscale[band + 1]);
    float n = 0;

    if (b < a) {
        n += freq[b] * (xscale[band + 1] - xscale[band]);
    } else {
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

static void computeLogXscale(float *xscale, int bands)
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
    computeLogXscale(m_xscale, N_BANDS);
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

const unsigned int BG_DOT_SIZE = 1 * UI_SCALE;
const unsigned int BG_DOT_SPACING = 1 * UI_SCALE;

void SpectrumWidget::paintBackground(QPainter & p)
{
    // Paint the gray pixels behind the spectrum
    // Pixels: 1px*3 x 1px*3
    // Spacing: 1px*3 for both x and y
    const unsigned int rows = 8;
    const unsigned int cols = 38;

    // Starting points
    unsigned int x = BG_DOT_SIZE;
    unsigned int y = BG_DOT_SIZE + BG_DOT_SPACING;

    const QColor color = QColor::fromRgb(64, 64, 64);

    for(unsigned int row = 0; row < rows; row++) {
        for(unsigned int col = 0; col < cols; col++) {
            p.fillRect(x, y, BG_DOT_SIZE, BG_DOT_SIZE, color);
            x += BG_DOT_SIZE + BG_DOT_SPACING; // Add pixel width + spacing
        }
        x = BG_DOT_SIZE;  // Go back to the start of the row
        y += BG_DOT_SIZE + BG_DOT_SPACING; // Add pixel height + spacing
    }
}

const unsigned int BAR_W = 3 * UI_SCALE;
const unsigned int BAR_SPACING = 1 * UI_SCALE;

void SpectrumWidget::paintSpectrum (QPainter & p)
{
    for (int i = 0; i < N_BANDS; i++) {
        // Bar measures 3px*3 wide, 1px*3 spacing
        int x = (BAR_W * i) + BAR_SPACING*i;
        p.fillRect(x + BAR_SPACING, height() - (m_bandValues[i] * height() / 40),
                   BAR_W, (m_bandValues[i] * height() / 40), *getSpecBarGradient());
    }
}

void SpectrumWidget::paintPeaks (QPainter & p)
{
    const QColor color = QColor::fromRgb(191, 191, 191);
    for (int i = 0; i < N_BANDS; i++) {
        // Peak rectangle measures 3px*3 wide, 1px*3 high, 1px*3 spacing
        int x = (BAR_W * i) + BAR_SPACING*i;
        p.fillRect(x + BAR_SPACING, height() - (m_peakValues[i] * height() / 40),
                   BAR_W, BAR_SPACING, color);
    }
}


void SpectrumWidget::paintEvent (QPaintEvent *)
{
    QPainter p(this);

    paintBackground(p);

    if(m_playing) {
        float mono[N];
        float freq[N / 2];
        int channels = m_format.channelCount();
        floatPcmToMono(m_data, mono, channels);
        calc_freq(mono, freq);

        for(int i = 0; i < N_BANDS; i ++) {
            /* 40 dB range */
            int x = 40 + computeFreqBand(freq, m_xscale, i, N_BANDS);
            x = std::clamp(x, 0, 40);

            m_bandValues[i] -= std::max(0, VIS_FALLOFF - m_bandDelays[i]);

            if (m_bandDelays[i])
                m_bandDelays[i]--;

            if (x > m_bandValues[i]) {
                m_bandValues[i] = x;
                m_bandDelays[i] = VIS_DELAY;
            }

            m_peakValues[i] -= std::max(0, VIS_PEAK_FALLOFF - m_peakDelays[i]);

            if (m_peakDelays[i])
                m_peakDelays[i]--;

            if (x > m_peakValues[i]) {
                m_peakValues[i] = x;
                m_peakDelays[i] = VIS_PEAK_DELAY;
            }
        }

        paintSpectrum(p);
        paintPeaks(p);
    }
}

void SpectrumWidget::setData(const QByteArray &data, QAudioFormat format)
{
    m_format = format;
    Q_ASSERT(m_format.sampleFormat() == QAudioFormat::Int16);

    if(m_format.sampleFormat() != QAudioFormat::Int16) {
        // Bad sample format, only Int16 is currently supported
        return;
    }

    const int bytesPerFrame = format.bytesPerFrame();
    Q_ASSERT(bytesPerFrame == 4); // Expecting Stereo Int16

    if(bytesPerFrame != 4) {
        // Bad bytes per frame, expecting 4 (Int16 Stereo)
        return;
    }

    if(data.length() < DFT_SIZE * bytesPerFrame) {
        // Not enough data for processing, ignore
        return;
    }

    const char *ptr = data.constData();
    for (int i = 0; i < DFT_SIZE * bytesPerFrame; ++i) {
        const qint16 pcmSample = *reinterpret_cast<const qint16 *>(ptr);
        // Scale down to range [-1.0, 1.0]
        float floatSample = pcmToFloat(pcmSample);
        m_data[i] = floatSample;
        ptr += 2;
    }
}

void SpectrumWidget::clear() {
    memset(m_data, 0, sizeof m_data);
    memset(m_bandValues, 0, sizeof m_bandValues);
    memset(m_bandDelays, 0, sizeof m_bandDelays);
    memset(m_peakValues, 0, sizeof m_bandValues);
    memset(m_peakDelays, 0, sizeof m_bandDelays);
}

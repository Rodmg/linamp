#ifndef SPECTRUMWIDGET_H
#define SPECTRUMWIDGET_H

#define DFT_SIZE 512  /* size of the DFT */
#define N_BANDS 19

#include <QWidget>
#include <QTimer>
#include <QMutex>

class SpectrumWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SpectrumWidget(QWidget *parent = nullptr);
    void play();
    void pause();
    void stop();

protected:
    void paintEvent (QPaintEvent *);

private:
    float m_data[DFT_SIZE];
    float m_xscale[N_BANDS + 1];
    int m_bandValues[N_BANDS + 1];
    int m_bandDelays[N_BANDS + 1];
    int m_peakValues[N_BANDS + 1];
    int m_peakDelays[N_BANDS + 1];
    bool m_playing = false;
    QTimer *m_renderTimer = nullptr;
    QMutex dataMutex;

    void paintBackground(QPainter &);
    void paintSpectrum(QPainter &);
    void paintPeaks(QPainter &);

    void clear();

public slots:
    void setData(const QByteArray& data);

signals:

};

#endif // SPECTRUMWIDGET_H

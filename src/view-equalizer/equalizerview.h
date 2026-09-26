#ifndef EQUALIZERVIEW_H
#define EQUALIZERVIEW_H

#include <QWidget>

class SystemEqualizer;
class EqualizerGraph;
class EqualizerPresetView;
class EqPreset;
class QCheckBox;
class QLabel;
class QSlider;
class QTimer;

class EqualizerView : public QWidget
{
    Q_OBJECT
public:
    explicit EqualizerView(SystemEqualizer *equalizer, QWidget *parent = nullptr);

signals:
    void backClicked();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    SystemEqualizer *m_equalizer = nullptr;
    EqualizerGraph *m_graph = nullptr;
    EqualizerPresetView *m_presetView = nullptr;
    QCheckBox *m_onButton = nullptr;
    QLabel *m_valueLabel = nullptr;
    QTimer *m_valueHideTimer = nullptr;
    QSlider *m_preampSlider = nullptr;
    QSlider *m_bandSliders[10] = {};
    bool m_applyingPreset = false;

    void showValue(const QString &name, double db);
    QSlider *makeSlider(double db);
    void applyPreset(const EqPreset &preset);
    void saveCurrentPreset();
    void updateCurrentPreset();
    EqPreset snapshotGains() const;
};

#endif // EQUALIZERVIEW_H

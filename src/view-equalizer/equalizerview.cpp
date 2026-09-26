#include "equalizerview.h"
#include "equalizergraph.h"
#include "equalizerpresetview.h"
#include "equalizerbands.h"
#include "equalizerpreset.h"
#include "systemequalizer.h"
#include "linampslider.h"
#include "scale.h"

#include <QCheckBox>
#include <QColor>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QList>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QSlider>
#include <QStyle>
#include <QStyleOptionSlider>
#include <QTimer>
#include <QVBoxLayout>

namespace {

class EqFader : public LinampSlider
{
public:
    using LinampSlider::LinampSlider;

    QRect handleRectFor(int value) const
    {
        QStyleOptionSlider opt;
        initStyleOption(&opt);
        opt.sliderPosition = value;
        opt.sliderValue = value;
        return style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
    }

    QRect grooveRect() const
    {
        QStyleOptionSlider opt;
        initStyleOption(&opt);
        return style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
    }
};

class EqGuideOverlay : public QWidget
{
public:
    explicit EqGuideOverlay(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TranslucentBackground);
    }

    EqFader *preamp = nullptr;
    EqFader *bands[EqBands::Count] = {};
    QWidget *scaleSlot = nullptr;

protected:
    void paintEvent(QPaintEvent *) override
    {
        if (preamp == nullptr || bands[0] == nullptr || scaleSlot == nullptr) {
            return;
        }

        const int yTop = preamp->mapTo(parentWidget(), preamp->handleRectFor(EqBands::SliderMax).center()).y();
        const int yMid = preamp->mapTo(parentWidget(), preamp->handleRectFor(0).center()).y();
        const int yBot = preamp->mapTo(parentWidget(), preamp->handleRectFor(EqBands::SliderMin).center()).y();

        EqFader *faders[1 + EqBands::Count];
        faders[0] = preamp;
        for (int i = 0; i < EqBands::Count; ++i) {
            faders[i + 1] = bands[i];
        }

        QRegion clip(rect());
        for (EqFader *fader : faders) {
            const QRect handle = fader->handleRectFor(fader->value());
            clip -= QRect(fader->mapTo(parentWidget(), handle.topLeft()), handle.size()).adjusted(-1, -1, 1, 1);
        }

        QPainter painter(this);
        painter.setClipRegion(clip);
        painter.setPen(QPen(Qt::white, 2));
        constexpr int kTickInset = 10;
        constexpr int kShaftGap = 14;
        for (EqFader *fader : faders) {
            const QRect box(fader->mapTo(parentWidget(), QPoint(0, 0)), fader->size());
            const QRect groove(fader->mapTo(parentWidget(), fader->grooveRect().topLeft()), fader->grooveRect().size());
            const int left = box.left() + kTickInset;
            const int right = box.right() - kTickInset;
            for (int y : {yTop, yMid, yBot}) {
                if (groove.left() - kShaftGap > left) {
                    painter.drawLine(left, y, groove.left() - kShaftGap, y);
                }
                if (right > groove.right() + kShaftGap) {
                    painter.drawLine(groove.right() + kShaftGap, y, right, y);
                }
            }
        }
        painter.setClipping(false);

        QFont font("DejaVu Sans Mono", 11);
        font.setStyleStrategy(QFont::NoAntialias);
        font.setBold(true);
        painter.setFont(font);
        painter.setPen(QColor("#ffe000"));

        const QRect slot(scaleSlot->mapTo(parentWidget(), QPoint(0, 0)), scaleSlot->size());
        auto drawDb = [&](int y, const QString &text) {
            painter.drawText(QRect(slot.left(), y - 10, slot.width(), 20), Qt::AlignCenter, text);
        };
        drawDb(yTop, "+12 dB");
        drawDb(yMid, "+0 dB");
        drawDb(yBot, "-12 dB");
    }
};

class EqFaderPanel : public QWidget
{
public:
    explicit EqFaderPanel(QWidget *parent = nullptr)
        : QWidget(parent)
        , overlay(new EqGuideOverlay(this))
    {
        setAttribute(Qt::WA_StyledBackground, false);
    }

    EqGuideOverlay *overlay = nullptr;

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);
        overlay->setGeometry(rect());
        overlay->raise();
    }
};

QFont pixelFont(int pointSize, bool bold = false)
{
    QFont font("DejaVu Sans Mono", pointSize);
    font.setStyleStrategy(QFont::NoAntialias);
    font.setBold(bold);
    return font;
}

QString eqFaderStyle()
{
    const int handle = 11 * UI_SCALE;
    const int grooveW = 6 * UI_SCALE;
    const int margin = (handle - grooveW) / 2;
    const int border = 1 * UI_SCALE;
    const QString radius = QString::number(2.66 * UI_SCALE, 'f', 2);
    return QString(R"(
        QSlider { background: transparent; }
        QSlider::handle:vertical {
            width: %1px;
            height: %1px;
            background-color: transparent;
            border-image: url(:assets/eq_handle.png);
            background: none;
            background-repeat: none;
            margin-left: -%2px;
            margin-right: -%2px;
        }
        QSlider::handle:vertical:pressed {
            border-image: url(:assets/eq_handle_p.png);
        }
        QSlider::groove:vertical {
            background: transparent;
            width: %3px;
        }
        QSlider::sub-page:vertical {
            background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1, stop: 0 #1c6c14, stop: 1 #28991c);
            border-top: %4px solid #161623;
            border-left: %4px solid #161623;
            border-bottom: %4px solid #7d7d92;
            border-right: %4px solid #7d7d92;
            border-radius: %5px;
        }
        QSlider::add-page:vertical {
            width: %3px;
            background: qlineargradient(x1: 0, y1: 0, x2: 1, y2: 1, stop: 0 #1c6c14, stop: 1 #28991c);
            border-top: %4px solid #161623;
            border-left: %4px solid #161623;
            border-bottom: %4px solid #7d7d92;
            border-right: %4px solid #7d7d92;
            border-radius: %5px;
        }
    )").arg(handle).arg(margin).arg(grooveW).arg(border).arg(radius);
}

QLabel *freqLabel(const QString &text, bool accent)
{
    auto *label = new QLabel(text);
    label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    label->setFont(pixelFont(12, true));
    label->setStyleSheet(accent ? "color: #00e800; background: transparent;" : "color: #d0d0e4; background: transparent;");
    label->setMinimumHeight(22);
    return label;
}

// Slider units are 0.1 dB, so 5 is ±0.5 dB.
constexpr int kZeroSnap = 5;

bool snapSliderToZero(QSlider *slider)
{
    if (!slider->isSliderDown()) {
        return false;
    }
    const int value = slider->value();
    if (value >= -kZeroSnap && value <= kZeroSnap && value != 0) {
        slider->setValue(0);
        return true;
    }
    return false;
}

}

EqualizerView::EqualizerView(SystemEqualizer *equalizer, QWidget *parent)
    : QWidget(parent)
    , m_equalizer(equalizer)
{
    setObjectName("EqualizerView");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("#EqualizerView { background-color: #2e2e48; }");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 10, 16, 8);
    root->setSpacing(6);

    auto *header = new QHBoxLayout;
    header->setSpacing(14);

    auto *titleBox = new QVBoxLayout;
    titleBox->setSpacing(6);

    m_onButton = new QCheckBox;
    m_onButton->setText(QString());
    m_onButton->setCursor(Qt::PointingHandCursor);
    m_onButton->setFixedSize(25 * UI_SCALE, 12 * UI_SCALE);
    m_onButton->setStyleSheet(QStringLiteral(
        "QCheckBox { spacing: 0px; }"
        "QCheckBox::indicator { width: %1px; height: %2px; }"
        "QCheckBox::indicator:unchecked { image: url(:assets/eq_toggle_off.png); }"
        "QCheckBox::indicator:unchecked:hover { image: url(:assets/eq_toggle_off.png); }"
        "QCheckBox::indicator:unchecked:pressed { image: url(:assets/eq_toggle_off_p.png); }"
        "QCheckBox::indicator:checked { image: url(:assets/eq_toggle_on.png); }"
        "QCheckBox::indicator:checked:hover { image: url(:assets/eq_toggle_on.png); }"
        "QCheckBox::indicator:checked:pressed { image: url(:assets/eq_toggle_on_p.png); }")
        .arg(25 * UI_SCALE)
        .arg(12 * UI_SCALE));
    m_onButton->setChecked(m_equalizer->isEnabled());
    titleBox->addWidget(m_onButton, 0, Qt::AlignLeft);

    const QFont valueFont = pixelFont(13, true);
    m_valueLabel = new QLabel;
    m_valueLabel->setFont(valueFont);
    m_valueLabel->setStyleSheet("color: #00e800; background: transparent;");
    m_valueLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    const int valueWidth = QFontMetrics(valueFont).horizontalAdvance("PRE  +12.0 dB");
    m_valueLabel->setFixedWidth(valueWidth);
    m_valueLabel->setFixedHeight(QFontMetrics(valueFont).height());
    titleBox->addWidget(m_valueLabel, 0, Qt::AlignLeft);
    titleBox->addStretch(1);
    header->addLayout(titleBox);

    m_valueHideTimer = new QTimer(this);
    m_valueHideTimer->setSingleShot(true);
    m_valueHideTimer->setInterval(1500);
    connect(m_valueHideTimer, &QTimer::timeout, this, [this] {
        m_valueLabel->clear();
    });

    m_graph = new EqualizerGraph(this);
    double bands[EqBands::Count];
    for (int i = 0; i < EqBands::Count; ++i) {
        bands[i] = m_equalizer->bandDb(i);
    }
    m_graph->setBands(bands, EqBands::Count);
    m_graph->setPreampDb(m_equalizer->preampDb());
    auto *graphSlot = new QHBoxLayout;
    graphSlot->setContentsMargins(64, 0, 64, 0);
    graphSlot->setSpacing(0);
    graphSlot->addWidget(m_graph);
    header->addLayout(graphSlot, 1);

    auto *presetsButton = new QPushButton;
    presetsButton->setCursor(Qt::PointingHandCursor);
    presetsButton->setFixedSize(44 * UI_SCALE, 12 * UI_SCALE);
    presetsButton->setStyleSheet(
        "QPushButton {"
        "  border: 0px;"
        "  background: transparent;"
        "  image: url(:assets/eq_presets.png);"
        "}"
        "QPushButton:pressed {"
        "  image: url(:assets/eq_presets_p.png);"
        "}");
    header->addWidget(presetsButton, 0, Qt::AlignTop);

    auto *backButton = new QPushButton;
    backButton->setIcon(QIcon(":/assets/menu-icon-x.png"));
    backButton->setIconSize(QSize(44, 44));
    backButton->setFixedSize(48, 64);
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setStyleSheet(
        "QPushButton { background-color: transparent; border: 0px; border-radius: 0px; }"
        "QPushButton:pressed { background-color: #4A4A71; }");
    header->addWidget(backButton);
    root->addLayout(header);

    auto *panel = new EqFaderPanel(this);
    auto *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(0, 0, 0, 0);
    panelLayout->setSpacing(2);

    auto *sliderRow = new QHBoxLayout;
    sliderRow->setSpacing(4);
    auto *labelRow = new QHBoxLayout;
    labelRow->setSpacing(4);

    m_preampSlider = makeSlider(m_equalizer->preampDb());
    sliderRow->addWidget(m_preampSlider, 1);
    labelRow->addWidget(freqLabel("PREAMP", false), 1);

    auto *scaleSlot = new QWidget;
    scaleSlot->setFixedWidth(84);
    sliderRow->addWidget(scaleSlot);
    auto *scaleLabelGap = new QWidget;
    scaleLabelGap->setFixedWidth(84);
    labelRow->addWidget(scaleLabelGap);

    for (int i = 0; i < EqBands::Count; ++i) {
        m_bandSliders[i] = makeSlider(m_equalizer->bandDb(i));
        sliderRow->addWidget(m_bandSliders[i], 1);
        labelRow->addWidget(freqLabel(EqBands::label(i), false), 1);
    }

    panelLayout->addLayout(sliderRow, 1);
    panelLayout->addLayout(labelRow);
    root->addWidget(panel, 1);

    panel->overlay->preamp = static_cast<EqFader *>(m_preampSlider);
    for (int i = 0; i < EqBands::Count; ++i) {
        panel->overlay->bands[i] = static_cast<EqFader *>(m_bandSliders[i]);
    }
    panel->overlay->scaleSlot = scaleSlot;
    panel->overlay->raise();

    connect(backButton, &QPushButton::clicked, this, &EqualizerView::backClicked);
    connect(m_onButton, &QCheckBox::toggled, m_equalizer, &SystemEqualizer::setEnabled);
    connect(m_equalizer, &SystemEqualizer::enabledChanged, this, [this](bool enabled) {
        if (m_onButton->isChecked() != enabled) {
            m_onButton->blockSignals(true);
            m_onButton->setChecked(enabled);
            m_onButton->blockSignals(false);
        }
    });

    auto refreshGuides = [panel] {
        panel->overlay->update();
    };

    connect(m_preampSlider, &QSlider::valueChanged, this, [this, refreshGuides](int value) {
        if (snapSliderToZero(m_preampSlider)) {
            return;
        }
        const double db = EqBands::sliderToDb(value);
        m_equalizer->setPreampDb(db);
        m_graph->setPreampDb(db);
        showValue("PRE", db);
        refreshGuides();
        if (!m_applyingPreset && m_presetView != nullptr) {
            m_presetView->onManualChange();
        }
    });
    connect(m_preampSlider, &QSlider::sliderReleased, m_equalizer, &SystemEqualizer::flush);

    for (int i = 0; i < EqBands::Count; ++i) {
        connect(m_bandSliders[i], &QSlider::valueChanged, this, [this, i, refreshGuides](int value) {
            if (snapSliderToZero(m_bandSliders[i])) {
                return;
            }
            const double db = EqBands::sliderToDb(value);
            m_equalizer->setBandDb(i, db);
            m_graph->setBandDb(i, db);
            showValue(EqBands::label(i), db);
            refreshGuides();
            if (!m_applyingPreset && m_presetView != nullptr) {
                m_presetView->onManualChange();
            }
        });
        connect(m_bandSliders[i], &QSlider::sliderReleased, m_equalizer, &SystemEqualizer::flush);
    }

    m_presetView = new EqualizerPresetView(this);
    m_presetView->hide();
    connect(presetsButton, &QPushButton::clicked, this, [this] {
        m_presetView->setGeometry(rect());
        m_presetView->show();
        m_presetView->raise();
    });
    connect(m_presetView, &EqualizerPresetView::presetChosen, this, &EqualizerView::applyPreset);
    connect(m_presetView, &EqualizerPresetView::saveClicked, this, &EqualizerView::saveCurrentPreset);
    connect(m_presetView, &EqualizerPresetView::updateClicked, this, &EqualizerView::updateCurrentPreset);
}

void EqualizerView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_presetView != nullptr) {
        m_presetView->setGeometry(rect());
    }
}

void EqualizerView::applyPreset(const EqPreset &preset)
{
    m_applyingPreset = true;
    m_equalizer->setEnabled(true);
    m_preampSlider->setValue(EqBands::dbToSlider(preset.preampDb));
    for (int i = 0; i < EqBands::Count; ++i) {
        m_bandSliders[i]->setValue(EqBands::dbToSlider(preset.bandDb[i]));
    }
    m_equalizer->flush();
    m_applyingPreset = false;
}

void EqualizerView::saveCurrentPreset()
{
    m_presetView->saveFrom(snapshotGains());
}

void EqualizerView::updateCurrentPreset()
{
    m_presetView->updateSelected(snapshotGains());
}

EqPreset EqualizerView::snapshotGains() const
{
    EqPreset preset;
    preset.preampDb = m_equalizer->preampDb();
    for (int i = 0; i < EqBands::Count; ++i) {
        preset.bandDb[i] = m_equalizer->bandDb(i);
    }
    return preset;
}

void EqualizerView::showValue(const QString &name, double db)
{
    const QString sign = db > 0.05 ? "+" : "";
    m_valueLabel->setText(QString("%1  %2%3 dB").arg(name, sign).arg(db, 0, 'f', 1));
    m_valueHideTimer->start();
}

QSlider *EqualizerView::makeSlider(double db)
{
    static const QList<QColor> kFaderGradient{
        QColor::fromRgb(7, 191, 0),
        QColor::fromRgb(28, 191, 0),
        QColor::fromRgb(59, 191, 0),
        QColor::fromRgb(95, 191, 0),
        QColor::fromRgb(132, 191, 0),
        QColor::fromRgb(163, 191, 0),
        QColor::fromRgb(183, 191, 0),
        QColor::fromRgb(191, 191, 0),
        QColor::fromRgb(191, 183, 0),
        QColor::fromRgb(191, 163, 0),
        QColor::fromRgb(191, 132, 0),
        QColor::fromRgb(191, 95, 0),
        QColor::fromRgb(191, 59, 0),
        QColor::fromRgb(191, 28, 0),
        QColor::fromRgb(191, 7, 0),
        QColor::fromRgb(192, 0, 0),
    };

    auto *slider = new EqFader(this);
    slider->setRange(EqBands::SliderMin, EqBands::SliderMax);
    slider->setSingleStep(1);
    slider->setPageStep(10);
    slider->setTracking(true);
    slider->setFocusPolicy(Qt::NoFocus);
    slider->setStyleSheet(eqFaderStyle());
    slider->setGradient(kFaderGradient, Qt::Vertical);
    slider->setValue(EqBands::dbToSlider(db));
    slider->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    slider->setMinimumWidth(56);
    return slider;
}

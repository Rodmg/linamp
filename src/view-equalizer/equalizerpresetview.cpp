#include "equalizerpresetview.h"
#include "equalizerbands.h"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QIcon>
#include <QItemSelectionModel>
#include <QListView>
#include <QPushButton>
#include <QScroller>
#include <QScrollerProperties>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QVariant>

namespace {

constexpr int kPresetRole = Qt::UserRole;
constexpr int kSectionRole = Qt::UserRole + 1;

QString chromeButtonStyle()
{
    return QStringLiteral(
        "QPushButton {"
        "  color: #080810;"
        "  background-color: #bdced6;"
        "  border: 4px solid #4a5a6b;"
        "  border-radius: 0px;"
        "}"
        "QPushButton:pressed {"
        "  color: #080810;"
        "  background-color: #7b8c9c;"
        "  border: 4px solid #080810;"
        "}"
        "QPushButton:disabled {"
        "  color: #4a5a6b;"
        "  background-color: #8a9aa6;"
        "  border: 4px solid #4a5a6b;"
        "}");
}

void grabPlaylistScroller(QScroller *scroller, QWidget *target)
{
    QScrollerProperties sp;
    sp.setScrollMetric(QScrollerProperties::DragVelocitySmoothingFactor, 0.6);
    sp.setScrollMetric(QScrollerProperties::MinimumVelocity, 0.0);
    sp.setScrollMetric(QScrollerProperties::MaximumVelocity, 0.5);
    sp.setScrollMetric(QScrollerProperties::AcceleratingFlickMaximumTime, 0.4);
    sp.setScrollMetric(QScrollerProperties::AcceleratingFlickSpeedupFactor, 1.2);
    sp.setScrollMetric(QScrollerProperties::SnapPositionRatio, 0.2);
    sp.setScrollMetric(QScrollerProperties::MaximumClickThroughVelocity, 0);
    sp.setScrollMetric(QScrollerProperties::DragStartDistance, 0.001);
    sp.setScrollMetric(QScrollerProperties::MousePressEventDelay, 1.0);
    sp.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor, 0.3);
    sp.setScrollMetric(QScrollerProperties::OvershootScrollDistanceFactor, 0.1);
    scroller->grabGesture(target, QScroller::LeftMouseButtonGesture);
    scroller->setScrollerProperties(sp);
}

QStandardItem *sectionItem(const QString &title)
{
    auto *item = new QStandardItem(title);
    item->setFlags(Qt::ItemIsEnabled);
    item->setData(true, kSectionRole);
    item->setForeground(QColor("#8d8da3"));
    item->setSizeHint(QSize(0, 45));
    return item;
}

QStandardItem *presetItem(const EqPreset &preset)
{
    auto *item = new QStandardItem(preset.name);
    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    item->setData(QVariant::fromValue(preset), kPresetRole);
    item->setData(false, kSectionRole);
    item->setForeground(preset.custom ? QColor("#7dff3a") : QColor("#00e800"));
    item->setSizeHint(QSize(0, 45));
    return item;
}

}

EqualizerPresetView::EqualizerPresetView(QWidget *parent)
    : QWidget(parent)
    , m_builtins(EqPresetStore::loadBuiltins())
    , m_custom(EqPresetStore::loadCustom())
{
    setObjectName("EqualizerPresetView");
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet("#EqualizerPresetView { background-color: #333350; }");

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(8);

    m_model = new QStandardItemModel(this);
    m_list = new QListView(this);
    QFont listFont("Bitstream Vera Sans Mono", 14);
    listFont.setBold(true);
    m_list->setFont(listFont);
    m_list->setModel(m_model);
    m_list->setUniformItemSizes(true);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_list->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_list->setFocusPolicy(Qt::NoFocus);
    m_list->setStyleSheet(QStringLiteral(
        "QListView {"
        "  color: #00e800;"
        "  background-color: #000000;"
        "  border-top: 3px solid #26253c;"
        "  border-right: 3px solid #6d6d7f;"
        "  border-bottom: 3px solid #6d6d7f;"
        "  border-left: 3px solid #26253c;"
        "  outline: none;"
        "}"
        "QListView::item { min-height: 45px; padding-left: 16px; padding-right: 16px; }"
        "QListView::item:selected { color: #000000; background-color: #00e800; }"
        "QListView::item:selected:!active { color: #000000; background-color: #00e800; }"
        "QScrollBar:vertical { background-color: #191926; width: 4px; }"
        "QScrollBar::handle:vertical { background-color: #b0995e; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        "  background: none; height: 0px; width: 0px; border: none;"
        "}"
        "QScrollBar::up-arrow:vertical, QScrollBar::down-arrow:vertical {"
        "  height: 0px; width: 0px; background: none; border: none;"
        "}"));
    root->addWidget(m_list, 1);

    m_scroller = QScroller::scroller(m_list);
    grabPlaylistScroller(m_scroller, m_list);

    auto *actions = new QVBoxLayout;
    actions->setSpacing(8);

    QFont buttonFont("Bitstream Vera Sans Mono", 12);
    buttonFont.setBold(true);

    auto *saveButton = new QPushButton("SAVE NEW");
    saveButton->setFixedSize(100, 54);
    saveButton->setCursor(Qt::PointingHandCursor);
    saveButton->setFont(buttonFont);
    saveButton->setStyleSheet(chromeButtonStyle());
    m_saveButton = saveButton;
    actions->addWidget(m_saveButton);

    m_updateButton = new QPushButton("UPDATE");
    m_updateButton->setFixedSize(100, 54);
    m_updateButton->setCursor(Qt::PointingHandCursor);
    m_updateButton->setFont(buttonFont);
    m_updateButton->setStyleSheet(chromeButtonStyle());
    m_updateButton->setEnabled(false);
    actions->addWidget(m_updateButton);

    m_deleteButton = new QPushButton("DELETE");
    m_deleteButton->setFixedSize(100, 54);
    m_deleteButton->setCursor(Qt::PointingHandCursor);
    m_deleteButton->setFont(buttonFont);
    m_deleteButton->setStyleSheet(chromeButtonStyle());
    m_deleteButton->setEnabled(false);
    actions->addWidget(m_deleteButton);
    actions->addStretch(1);
    root->addLayout(actions);

    auto *closeButton = new QPushButton;
    closeButton->setIcon(QIcon(":/assets/menu-icon-x.png"));
    closeButton->setIconSize(QSize(44, 44));
    closeButton->setFixedSize(48, 64);
    closeButton->setCursor(Qt::PointingHandCursor);
    closeButton->setStyleSheet(
        "QPushButton { background-color: transparent; border: 0px; border-radius: 0px; }"
        "QPushButton:pressed { background-color: #4A4A71; }");
    root->addWidget(closeButton, 0, Qt::AlignTop);

    connect(closeButton, &QPushButton::clicked, this, [this] {
        hide();
        emit closed();
    });
    connect(m_saveButton, &QPushButton::clicked, this, [this] {
        emit saveClicked();
    });
    connect(m_updateButton, &QPushButton::clicked, this, [this] {
        emit updateClicked();
    });
    connect(m_deleteButton, &QPushButton::clicked, this, [this] {
        const QModelIndex index = m_list->currentIndex();
        const EqPreset preset = presetAt(index);
        if (!preset.custom) {
            return;
        }
        for (int i = 0; i < m_custom.size(); ++i) {
            if (m_custom[i].name == preset.name) {
                m_custom.removeAt(i);
                break;
            }
        }
        EqPresetStore::saveCustom(m_custom);
        m_dirty = false;
        rebuildList();
    });
    connect(m_list, &QListView::clicked, this, [this](const QModelIndex &index) {
        if (index.data(kSectionRole).toBool()) {
            return;
        }
        const EqPreset preset = presetAt(index);
        if (preset.name.isEmpty()) {
            return;
        }
        m_dirty = false;
        updateActionButtons();
        emit presetChosen(preset);
    });
    connect(m_list->selectionModel(), &QItemSelectionModel::currentChanged, this, [this] {
        updateActionButtons();
    });

    rebuildList();
}

void EqualizerPresetView::saveFrom(const EqPreset &gains)
{
    EqPreset preset = gains;
    preset.custom = true;
    preset.name = EqPresetStore::nextCustomName(m_custom);
    m_custom.append(preset);
    EqPresetStore::saveCustom(m_custom);
    m_dirty = false;
    rebuildList(preset.name);
}

void EqualizerPresetView::updateSelected(const EqPreset &gains)
{
    const EqPreset selected = presetAt(m_list->currentIndex());
    if (!selected.custom) {
        return;
    }
    for (int i = 0; i < m_custom.size(); ++i) {
        if (m_custom[i].name != selected.name) {
            continue;
        }
        m_custom[i].preampDb = gains.preampDb;
        for (int band = 0; band < EqBands::Count; ++band) {
            m_custom[i].bandDb[band] = gains.bandDb[band];
        }
        break;
    }
    EqPresetStore::saveCustom(m_custom);
    m_dirty = false;
    rebuildList(selected.name);
}

void EqualizerPresetView::onManualChange()
{
    const EqPreset selected = presetAt(m_list->currentIndex());
    if (selected.custom) {
        m_dirty = true;
        updateActionButtons();
        return;
    }
    m_dirty = false;
    clearSelection();
}

void EqualizerPresetView::clearSelection()
{
    m_dirty = false;
    m_list->clearSelection();
    m_list->setCurrentIndex(QModelIndex());
    updateActionButtons();
}

void EqualizerPresetView::rebuildList(const QString &selectName)
{
    m_model->clear();
    m_model->appendRow(sectionItem("DEFAULT"));
    for (const EqPreset &preset : m_builtins) {
        m_model->appendRow(presetItem(preset));
    }
    if (!m_custom.isEmpty()) {
        m_model->appendRow(sectionItem("CUSTOM"));
        for (const EqPreset &preset : m_custom) {
            m_model->appendRow(presetItem(preset));
        }
    }

    if (!selectName.isEmpty()) {
        for (int row = 0; row < m_model->rowCount(); ++row) {
            const QModelIndex index = m_model->index(row, 0);
            if (index.data(kSectionRole).toBool()) {
                continue;
            }
            if (presetAt(index).name == selectName) {
                m_list->setCurrentIndex(index);
                m_list->scrollTo(index);
                break;
            }
        }
    }
    updateActionButtons();
}

void EqualizerPresetView::updateActionButtons()
{
    const EqPreset selected = presetAt(m_list->currentIndex());
    const bool presetSelected = !selected.name.isEmpty();
    m_saveButton->setEnabled(!presetSelected);
    m_updateButton->setEnabled(selected.custom && m_dirty);
    m_deleteButton->setEnabled(selected.custom);
}

EqPreset EqualizerPresetView::presetAt(const QModelIndex &index) const
{
    if (!index.isValid() || index.data(kSectionRole).toBool()) {
        return {};
    }
    return index.data(kPresetRole).value<EqPreset>();
}

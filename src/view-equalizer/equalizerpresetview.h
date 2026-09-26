#ifndef EQUALIZERPRESETVIEW_H
#define EQUALIZERPRESETVIEW_H

#include "equalizerpreset.h"

#include <QModelIndex>
#include <QWidget>

class QListView;
class QPushButton;
class QStandardItemModel;
class QScroller;

class EqualizerPresetView : public QWidget
{
    Q_OBJECT
public:
    explicit EqualizerPresetView(QWidget *parent = nullptr);

    void saveFrom(const EqPreset &gains);
    void updateSelected(const EqPreset &gains);
    void onManualChange();
    void clearSelection();

signals:
    void closed();
    void saveClicked();
    void updateClicked();
    void presetChosen(const EqPreset &preset);

private:
    QListView *m_list = nullptr;
    QStandardItemModel *m_model = nullptr;
    QPushButton *m_saveButton = nullptr;
    QPushButton *m_updateButton = nullptr;
    QPushButton *m_deleteButton = nullptr;
    QScroller *m_scroller = nullptr;
    QVector<EqPreset> m_builtins;
    QVector<EqPreset> m_custom;
    bool m_dirty = false;

    void rebuildList(const QString &selectName = QString());
    void updateActionButtons();
    EqPreset presetAt(const QModelIndex &index) const;
};

#endif // EQUALIZERPRESETVIEW_H

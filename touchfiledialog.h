#ifndef TOUCHFILEDIALOG_H
#define TOUCHFILEDIALOG_H

#include <QFileDialog>
#include <QObject>

class EventFilterCtrlModifier : public QObject
{
    Q_OBJECT;
public:
    EventFilterCtrlModifier( QObject* parent);
protected:
    virtual bool eventFilter( QObject* watched, QEvent* e);
};

class TouchFileDialog : public QFileDialog
{
    Q_OBJECT

    EventFilterCtrlModifier* listViewEventFilter;
    EventFilterCtrlModifier* treeViewEventFilter;

    bool initialized;

public:
    TouchFileDialog(QWidget* parent);

protected:
    virtual void showEvent( QShowEvent* e);

private:
    void setupMultipleSelection();
};

#endif // TOUCHFILEDIALOG_H

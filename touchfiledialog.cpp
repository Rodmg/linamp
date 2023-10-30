#include "touchfiledialog.h"
#include <QEvent>
#include <QMouseEvent>
#include <QCoreApplication>
#include <QListView>
#include <QTreeView>

EventFilterCtrlModifier::EventFilterCtrlModifier(QObject* parent)
    : QObject(parent)
{

}

bool EventFilterCtrlModifier::eventFilter(QObject* watched, QEvent* e)
{
    QEvent::Type type = e->type();

    if( type == QEvent::MouseButtonPress || type == QEvent::MouseButtonRelease )
    {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(e);

        // Create and post a new event with ctrl modifier if the event does not already have one.
        if( !mouseEvent->modifiers().testFlag(Qt::ControlModifier) )
        {
            QMouseEvent* newEventWithModifier = mouseEvent->clone();
            newEventWithModifier->setModifiers(newEventWithModifier->modifiers() | Qt::ControlModifier);

            QCoreApplication::postEvent(watched, newEventWithModifier);

            return true; // absorb the original event
        }
    }

    return false;
}


TouchFileDialog::TouchFileDialog(QWidget* parent)
    : QFileDialog(parent)
    , listViewEventFilter(NULL)
    , treeViewEventFilter(NULL)
    , initialized(false)
{
}

void TouchFileDialog::showEvent(QShowEvent* e)
{
    // install objects that are needed for multiple file selection if needed
    if( !initialized )
    {
        if( fileMode() == QFileDialog::ExistingFiles )
        {
            setupMultipleSelection();
        }

        initialized = true;
    }

    QFileDialog::showEvent(e);
}

void TouchFileDialog::setupMultipleSelection()
{
    // install event filter to item views that are used to add ctrl modifiers to mouse events
    listViewEventFilter = new EventFilterCtrlModifier(this);
    QListView* listView = findChild<QListView*>();
    listView->viewport()->installEventFilter(listViewEventFilter);

    treeViewEventFilter = new EventFilterCtrlModifier(this);
    QTreeView* treeView = findChild<QTreeView*>();
    treeView->viewport()->installEventFilter(treeViewEventFilter);
}

#ifndef CONTROLBUTTONSWIDGET_H
#define CONTROLBUTTONSWIDGET_H

#include <QWidget>

namespace Ui {
class ControlButtonsWidget;
}

class ControlButtonsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ControlButtonsWidget(QWidget *parent = nullptr);
    ~ControlButtonsWidget();

private:
    Ui::ControlButtonsWidget *ui;
    void scale();

signals:
    void playClicked();
    void pauseClicked();
    void stopClicked();
    void nextClicked();
    void previousClicked();
    void openClicked();
    void repeatClicked(bool checked);
    void shuffleClicked(bool checked);
};

#endif // CONTROLBUTTONSWIDGET_H

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

    void setShuffleEnabled(bool enabled);
    void setRepeatEnabled(bool enabled);

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
    void logoClicked();
};

#endif // CONTROLBUTTONSWIDGET_H

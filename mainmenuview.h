#ifndef MAINMENUVIEW_H
#define MAINMENUVIEW_H

#include <QWidget>
#include <QProcess>

namespace Ui {
class MainMenuView;
}

class MainMenuView : public QWidget
{
    Q_OBJECT

public:
    explicit MainMenuView(QWidget *parent = nullptr);
    ~MainMenuView();

signals:
    void sourceSelected(int source);
    void backClicked();

private:
    Ui::MainMenuView *ui;

    void fileSourceClicked();
    void btSourceClicked();
    void spotifySourceClicked();

    QProcess *shutdownProcess = nullptr;
    void shutdown();
};

#endif // MAINMENUVIEW_H

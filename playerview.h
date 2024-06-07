#ifndef PLAYERVIEW_H
#define PLAYERVIEW_H

#include "controlbuttonswidget.h"
#include "mediaplayer.h"
#include "spectrumwidget.h"

#include <QMediaMetaData>
#include <QAudioOutput>
#include <QWidget>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QAbstractItemView;
class QLabel;
class QModelIndex;
class QPushButton;
class QComboBox;
class QSlider;
class QStatusBar;
QT_END_NAMESPACE

namespace Ui {
class PlayerView;
}

class PlayerView : public QWidget
{
    Q_OBJECT

public:
    explicit PlayerView(QWidget *parent = nullptr, ControlButtonsWidget *ctlBtns = nullptr);
    ~PlayerView();

    void setSourceLabel(QString label);

public slots:
    void setPlaybackState(MediaPlayer::PlaybackState state);
    void setPosition(qint64 progress);
    void setSpectrumData(const QByteArray& data, QAudioFormat format);
    void setMetadata(QMediaMetaData metadata);
    void setDuration(qint64 duration);
    void setVolume(int volume);
    void setBalance(int balance);
    void setEqEnabled(bool enabled);
    void setPlEnabled(bool enabled);
    void setShuffleEnabled(bool enabled);
    void setRepeatEnabled(bool enabled);
    void setMessage(QString message, qint64 timeout);
    void clearMessage();

signals:
    void volumeChanged(int volume);
    void balanceChanged(int balance);
    void positionChanged(qint64 progress);
    void eqClicked();
    void plClicked();
    void previousClicked();
    void playClicked();
    void pauseClicked();
    void stopClicked();
    void nextClicked();
    void openClicked();
    void shuffleClicked();
    void repeatClicked();
    void menuClicked();

private:
    Ui::PlayerView *ui;
    void scale();
    SpectrumWidget *spectrum = nullptr;
    QTimer *messageTimer = nullptr;
    ControlButtonsWidget *controlButtons = nullptr;

    QString m_trackInfo;
    QString m_statusInfo;
    qint64 m_duration;

    bool plEnabled = false;
    bool eqEnabled = false;
    bool shuffleEnabled = false;
    bool repeatEnabled = false;

    void updateDurationInfo(qint64 currentInfo);
    void setTrackInfo(const QString &info);

private slots:
    void handleBalanceChanged();

};

#endif // PLAYERVIEW_H

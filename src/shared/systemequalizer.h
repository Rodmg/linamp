#ifndef SYSTEMEQUALIZER_H
#define SYSTEMEQUALIZER_H

#include <QObject>
#include <QTimer>
#include <QHash>
#include <QSettings>

class SystemEqualizer : public QObject
{
    Q_OBJECT
public:
    explicit SystemEqualizer(QObject *parent = nullptr);
    ~SystemEqualizer();

    bool isEnabled() const;
    double preampDb() const;
    double bandDb(int index) const;

public slots:
    void setEnabled(bool enabled);
    void setPreampDb(double db);
    void setBandDb(int index, double db);
    void flush();

signals:
    void enabledChanged(bool enabled);

private slots:
    void applyPending();

private:
    class Pw;
    Pw *m_pw = nullptr;

    QSettings m_settings;
    QTimer *m_applyTimer = nullptr;
    QHash<QString, double> m_pending;

    bool m_enabled = false;
    bool m_ready = false;
    double m_preampDb = 0.0;
    double m_bandDb[10] = {};

    void loadSettings();
    void saveEnabled();
    void savePreamp();
    void saveBand(int index);
    void queueParam(const QString &key, double value);
    void applyAll();
    bool startPipeWire();
};

#endif // SYSTEMEQUALIZER_H

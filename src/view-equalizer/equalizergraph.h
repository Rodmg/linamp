#ifndef EQUALIZERGRAPH_H
#define EQUALIZERGRAPH_H

#include <QColor>
#include <QWidget>

QColor equalizerColorForDb(double db);

class EqualizerGraph : public QWidget
{
    Q_OBJECT
public:
    explicit EqualizerGraph(QWidget *parent = nullptr);

    void setBandDb(int index, double db);
    void setBands(const double *db, int count);
    void setPreampDb(double db);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    double m_bandDb[10] = {};
    double m_preampDb = 0.0;
};

#endif // EQUALIZERGRAPH_H

#ifndef EQUALIZERPRESET_H
#define EQUALIZERPRESET_H

#include <QMetaType>
#include <QString>
#include <QVector>

struct EqPreset
{
    QString name;
    double preampDb = 0.0;
    double bandDb[10] = {};
    bool custom = false;
};

Q_DECLARE_METATYPE(EqPreset)

class EqPresetStore
{
public:
    static double eqfToDb(int eqf);
    static int dbToEqf(double db);

    static QVector<EqPreset> loadBuiltins();
    static QVector<EqPreset> loadCustom();
    static void saveCustom(const QVector<EqPreset> &presets);
    static QString nextCustomName(const QVector<EqPreset> &existing);
};

#endif // EQUALIZERPRESET_H

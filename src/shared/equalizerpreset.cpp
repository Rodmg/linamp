#include "equalizerpreset.h"
#include "equalizerbands.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QSettings>

#include <algorithm>
#include <cmath>

namespace {

const char *kHzKeys[EqBands::Count] = {
    "hz60", "hz170", "hz310", "hz600", "hz1000",
    "hz3000", "hz6000", "hz12000", "hz14000", "hz16000"
};

constexpr int kEqfMin = 1;
constexpr int kEqfMax = 64;

EqPreset fromJsonObject(const QJsonObject &obj, bool custom)
{
    EqPreset preset;
    preset.name = obj.value("name").toString();
    preset.custom = custom;
    preset.preampDb = EqPresetStore::eqfToDb(obj.value("preamp").toInt(33));
    for (int i = 0; i < EqBands::Count; ++i) {
        preset.bandDb[i] = EqPresetStore::eqfToDb(obj.value(kHzKeys[i]).toInt(33));
    }
    return preset;
}

QJsonObject toJsonObject(const EqPreset &preset)
{
    QJsonObject obj;
    obj.insert("name", preset.name);
    obj.insert("preamp", EqPresetStore::dbToEqf(preset.preampDb));
    for (int i = 0; i < EqBands::Count; ++i) {
        obj.insert(kHzKeys[i], EqPresetStore::dbToEqf(preset.bandDb[i]));
    }
    return obj;
}

QVector<EqPreset> presetsFromDocument(const QJsonDocument &doc, bool custom)
{
    QVector<EqPreset> out;
    const QJsonArray array = doc.object().value("presets").toArray();
    out.reserve(array.size());
    for (const QJsonValue &value : array) {
        if (!value.isObject()) {
            continue;
        }
        EqPreset preset = fromJsonObject(value.toObject(), custom);
        if (!preset.name.isEmpty()) {
            out.append(preset);
        }
    }
    return out;
}

}

double EqPresetStore::eqfToDb(int eqf)
{
    eqf = std::clamp(eqf, kEqfMin, kEqfMax);
    return EqBands::clampDb(((eqf - 1) / 63.0) * 24.0 - 12.0);
}

int EqPresetStore::dbToEqf(double db)
{
    db = EqBands::clampDb(db);
    const int eqf = int(std::lround((db + 12.0) / 24.0 * 63.0) + 1);
    return std::clamp(eqf, kEqfMin, kEqfMax);
}

QVector<EqPreset> EqPresetStore::loadBuiltins()
{
    QFile file(":/assets/eq-presets-builtin.json");
    if (!file.open(QFile::ReadOnly)) {
        return {};
    }
    return presetsFromDocument(QJsonDocument::fromJson(file.readAll()), false);
}

QVector<EqPreset> EqPresetStore::loadCustom()
{
    QSettings settings;
    settings.beginGroup("equalizer");
    const QByteArray json = settings.value("customPresets").toByteArray();
    settings.endGroup();
    if (json.isEmpty()) {
        return {};
    }
    return presetsFromDocument(QJsonDocument::fromJson(json), true);
}

void EqPresetStore::saveCustom(const QVector<EqPreset> &presets)
{
    QJsonArray array;
    for (const EqPreset &preset : presets) {
        array.append(toJsonObject(preset));
    }
    QJsonObject root;
    root.insert("type", "Winamp EQ library file v1.1");
    root.insert("presets", array);

    QSettings settings;
    settings.beginGroup("equalizer");
    settings.setValue("customPresets", QJsonDocument(root).toJson(QJsonDocument::Compact));
    settings.endGroup();
    settings.sync();
}

QString EqPresetStore::nextCustomName(const QVector<EqPreset> &existing)
{
    QSet<int> used;
    const QRegularExpression re("^Custom (\\d+)$");
    for (const EqPreset &preset : existing) {
        const QRegularExpressionMatch match = re.match(preset.name);
        if (match.hasMatch()) {
            used.insert(match.captured(1).toInt());
        }
    }
    int n = 1;
    while (used.contains(n)) {
        ++n;
    }
    return QString("Custom %1").arg(n);
}

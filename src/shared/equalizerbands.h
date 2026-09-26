#ifndef EQUALIZERBANDS_H
#define EQUALIZERBANDS_H

#include <algorithm>
#include <cmath>

namespace EqBands {

constexpr int Count = 10;
constexpr double Q = 1.4;
constexpr double MinDb = -12.0;
constexpr double MaxDb = 12.0;
constexpr int SliderMin = -120;
constexpr int SliderMax = 120;

constexpr double FrequenciesHz[Count] = {
    60, 170, 310, 600, 1000, 3000, 6000, 12000, 14000, 16000
};

inline const char *label(int index)
{
    static const char *labels[Count] = {
        "60", "170", "310", "600", "1K", "3K", "6K", "12K", "14K", "16K"
    };
    if (index < 0 || index >= Count) {
        return "";
    }
    return labels[index];
}

inline double clampDb(double db)
{
    if (db < MinDb) {
        return MinDb;
    }
    if (db > MaxDb) {
        return MaxDb;
    }
    return db;
}

inline int dbToSlider(double db)
{
    return std::clamp(int(std::lround(clampDb(db) * 10.0)), SliderMin, SliderMax);
}

inline double sliderToDb(int value)
{
    return clampDb(value / 10.0);
}

inline double dbToLinear(double db)
{
    return std::pow(10.0, clampDb(db) / 20.0);
}

}

#endif // EQUALIZERBANDS_H

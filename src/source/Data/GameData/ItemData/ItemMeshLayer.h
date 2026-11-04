#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace Data::Items
{
// [kind, base, amplitude, rate per millisecond, period, phase offset].
using ItemEffectCurve = std::array<double, 6>;
inline constexpr int RandomEffectCurve = 10;
inline constexpr int FrameGateEffectCurve = 11;

inline double EvaluateItemEffectCurve(const ItemEffectCurve& curve, double milliseconds, int random = 0)
{
    const int kind = static_cast<int>(curve[0]);
    if (kind == 0) return curve[1];
    if (kind == RandomEffectCurve) return curve[1] + curve[2] * (random % static_cast<int>(curve[4]));
    double phase = milliseconds * curve[3] + curve[5];
    if (kind == FrameGateEffectCurve)
    {
        const double frame = std::trunc(std::fmod(std::trunc(milliseconds), curve[4]) * curve[3]);
        return curve[1] + (frame == curve[5] ? curve[2] : 0.0);
    }
    if (kind == 8) phase = std::trunc(phase);
    if (curve[4] > 0.0) phase = std::fmod(phase, curve[4]);
    if (kind == 2 || kind == 5 || kind == 6) phase = std::sin(phase);
    if (kind == 5) phase = std::abs(phase);
    constexpr int AtlasColumns = 4;
    if (kind == 3) phase = static_cast<int>(phase) % AtlasColumns;
    if (kind == 4) phase = static_cast<int>(phase) / AtlasColumns;
    if (kind == 7 || kind == 9) phase = std::trunc(phase);
    const double value = curve[1] + curve[2] * phase;
    constexpr double SourcePulseCeiling = 0.8;
    return kind == 6 ? std::min(SourcePulseCeiling, value) : kind == 9 ? std::max(0.0, value) : value;
}

struct ItemMeshLayer
{
    int mesh = 0;
    int flags = 2;
    int blend = -1;
    ItemEffectCurve light{0, 1};
    ItemEffectCurve u{};
    ItemEffectCurve v{};
    std::array<ItemEffectCurve, 3> color{ItemEffectCurve{0, 1}, ItemEffectCurve{0, 1}, ItemEffectCurve{0, 1}};
    bool operator==(const ItemMeshLayer&) const = default;
};
}

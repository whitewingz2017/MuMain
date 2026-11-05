#pragma once

#include <array>

namespace Data::Items
{
struct ItemParticleEffect
{
    int bone = 0;
    int texture = 0;
    std::array<double, 3> color{};
    int subType = 0;
    double size = 1.0;
    std::array<double, 3> offset{};
    bool operator==(const ItemParticleEffect&) const = default;
};
}

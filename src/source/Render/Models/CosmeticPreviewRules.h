#pragma once

#include "GameLogic/Cosmetics/CosmeticArmor.h"
#include <array>

namespace Render::Models::CosmeticArmor
{
inline std::array<int, GameLogic::Cosmetics::SkinSlotCount> PreviewItems(int item)
{
    using namespace GameLogic::Cosmetics;
    std::array<int, SkinSlotCount> result;
    result.fill(-1);
    const int part = ArmorPart(item);
    if (part == 1)
        for (int body = 0; body < ArmorPartCount; ++body)
            result[body] = FullSetVisualItem(item, body);
    else if (part >= 0) result[part] = item;
    return result;
}
}

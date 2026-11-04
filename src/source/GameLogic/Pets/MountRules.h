#pragma once
#include "GameLogic/Cosmetics/FinalMuSkins.generated.h"
namespace GameLogic::Pets
{
constexpr int DarkHorseItem = 13 * 512 + 4;
constexpr int FenrirItem = 13 * 512 + 37;
constexpr bool IsMountItem(int type)
{
    const auto* skin = GameLogic::Cosmetics::FindFinalMuSkin(type);
    return type == DarkHorseItem || type == FenrirItem || (skin && skin->mount);
}
constexpr float MountViewScale(float scale, bool safeZone)
{
    constexpr float SafeZoneReduction = 0.5f;
    constexpr float MinimumScale = 0.1f;
    const float result = safeZone ? scale - SafeZoneReduction : scale;
    return result < MinimumScale ? MinimumScale : result;
}
}

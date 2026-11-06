#pragma once
#include "GameLogic/Cosmetics/CosmeticArmor.h"
namespace GameLogic::Pets
{
constexpr int DarkHorseItem = 13 * 512 + 4;
constexpr int FenrirItem = 13 * 512 + 37;
constexpr bool IsMountItem(int type)
{
    const auto* skin = GameLogic::Cosmetics::FindFinalMuSkin(type);
    return type == DarkHorseItem || type == FenrirItem || (skin && skin->mount);
}
// A Muun mount rides as the Dark Horse when it is one or a horse, else as Fenrir.
constexpr bool RidesAsDarkHorse(int type)
{
    return type == DarkHorseItem || GameLogic::Cosmetics::IsHorseMount(type);
}
// A mount walking beside its rider plays its walk: action 1 in the Fenrir's, the Dark Horse's and the
// FinalMU mounts' layouts. The followers' moving action (2) is the run of a ridden Fenrir and Dark Horse
// and no walk in the FinalMU mount models, which stood still in it while they moved.
constexpr int MountWalkAction = 1;
constexpr unsigned short MountFollowAction(unsigned short followerAction, int actionCount)
{
    return followerAction != 0 && actionCount > MountWalkAction ? static_cast<unsigned short>(MountWalkAction) : followerAction;
}
constexpr float MountViewScale(float scale, bool safeZone)
{
    constexpr float SafeZoneReduction = 0.5f;
    constexpr float MinimumScale = 0.1f;
    const float result = safeZone ? scale - SafeZoneReduction : scale;
    return result < MinimumScale ? MinimumScale : result;
}
}

#pragma once
#include "FinalMuSkins.generated.h"

namespace GameLogic::Cosmetics
{
inline constexpr int GloveSkinItem = 101;
inline constexpr int ScepterSkinItem = 2 * 512 + 101;
inline constexpr int BookSkinItem = 5 * 512 + 101;

// Stock model variants retain their original hand attachments and paired glove meshes.
constexpr int EquipmentVisualItem(int item)
{
    return item == GloveSkinItem ? 32 : item == ScepterSkinItem ? 2 * 512 + 8
        : item == BookSkinItem ? 5 * 512 + 21 : item;
}

constexpr int SkinWeaponFamily(int item)
{
    const auto* imported = FindFinalMuSkin(item);
    if (imported && imported->family) return imported->family;
    if (item == GloveSkinItem) return 8;
    if (item == ScepterSkinItem) return 6;
    if (item == BookSkinItem) return 7;
    const int group = item / 512;
    return group == 0 ? 1 : group == 1 ? 9 : group == 2 ? 2 : group == 3 ? 3
        : group == 4 ? 4 : group == 5 ? 5 : 0;
}
}

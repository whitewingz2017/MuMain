#pragma once

#include <cstdint>
#include <span>
#include <string>
#include "GameLogic/Cosmetics/FinalMuSkins.generated.h"
#include "GameLogic/Cosmetics/SkinWeaponRules.h"

namespace GameLogic::Cosmetics
{
constexpr int ArmorPartCount = 5;
constexpr int SkinSetNumber = 315;
constexpr int FirstArmorGroup = 7;
constexpr int FirstArmorEquipmentSlot = 2;

constexpr int SkinSlotCount = 9;
constexpr int CosmeticPetItemType = 13 * 512 + 200;
constexpr int NormalFenrirItemType = 13 * 512 + 37;
constexpr bool IsFenrirMount(int itemType)
{
    const auto* imported = FindFinalMuSkin(itemType);
    return imported && imported->mount;
}
constexpr int HelperBehaviorItem(int itemType)
{
    return IsFenrirMount(itemType) ? NormalFenrirItemType : itemType;
}
constexpr int EquipmentSlot(int part)
{
    return part >= 0 && part < ArmorPartCount ? part + FirstArmorEquipmentSlot
        : part == 5 ? 0 : part == 6 ? 1 : part == 7 ? 7 : part == 8 ? 8 : -1;
}
constexpr int SkinPartForEquipment(int slot)
{
    return slot == 3 ? 1
        : slot == 0 ? 5 : slot == 1 ? 6 : slot == 7 ? 7 : -1;
}
constexpr int ArmorPart(int itemType)
{
    constexpr int ItemsPerGroup = 512;
    if (itemType < 0) return -1;
    if (IsFenrirMount(itemType)) return -1;
    if (itemType == GloveSkinItem || itemType == ScepterSkinItem) return 5;
    if (itemType == BookSkinItem) return 6;
    const int group = itemType / ItemsPerGroup;
    const int number = itemType % ItemsPerGroup;
    if ((number == SkinSetNumber || number == 100) && group == 8) return 1;
    if (number == 100 && (group == 0 || group == 2 || group == 5)) return 5;
    if (number == 100 && (group == 4 || group == 6)) return 6;
    if (group == 12 && (number == 180 || number == 200 || number == 210 || number == 213)) return 7;
    if (itemType == CosmeticPetItemType) return 8;
    const auto* imported = FindFinalMuSkin(itemType);
    return imported ? imported->part : -1;
}
constexpr bool FitsPart(int itemType, int part)
{
    const int configured = ArmorPart(itemType);
    return configured >= 0 && (configured == part || (configured == 5 && part == 6));
}

constexpr int AvailablePart(int itemType, bool rightOccupied, bool leftOccupied)
{
    const int part = ArmorPart(itemType);
    return part == 5 && rightOccupied && !leftOccupied ? 6 : part;
}

constexpr bool IsFemaleArmor(int itemType)
{
    return itemType >= 0 && itemType % 512 == SkinSetNumber && itemType / 512 >= 7 && itemType / 512 <= 11;
}

constexpr bool IsCosmeticPet(int itemType)
{
    return itemType == CosmeticPetItemType || (FindFinalMuSkin(itemType) && FindFinalMuSkin(itemType)->part == 8 && !IsFenrirMount(itemType));
}

constexpr bool IsFloatingPet(int itemType)
{
    const auto* skin = FindFinalMuSkin(itemType);
    return itemType == CosmeticPetItemType || (skin && skin->floating);
}

constexpr float CosmeticPetHeight(int itemType)
{
    const auto* skin = FindFinalMuSkin(itemType);
    constexpr float OriginalPetHeight = 70.f;
    return skin ? skin->height : OriginalPetHeight;
}

constexpr int FullSetVisualItem(int itemType, int part)
{
    const auto* imported = FindFinalMuSkin(itemType);
    if (imported && part >= 0 && part < ArmorPartCount && !(imported->armorMask & (1 << part))) return -1;
    return ArmorPart(itemType) == 1 && part >= 0 && part < ArmorPartCount
        ? (FirstArmorGroup + part) * 512 + itemType % 512 : -1;
}

struct StoredSkin { int slot = -1; int type = -1; int level = 0; int flags = 0; };
std::span<const StoredSkin> StoredSkins();
bool StorageSupported();
bool Pending();
std::uint64_t Revision();
void Update();
bool Request(std::uint8_t operation, int inventorySlot = 0, int part = 0);
void Receive(std::span<const std::uint8_t> packet);
int EquippedArmor(int part);
int EquippedFlags(int part);
int VisualItem(int part);
std::span<const std::string> Statistics();
bool Ready();
void Reset();
}

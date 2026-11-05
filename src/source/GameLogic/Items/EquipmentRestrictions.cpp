#include "stdafx.h"
#include "GameLogic/Items/EquipmentRestrictions.h"

#include "GameLogic/Items/ItemCategories.h"
#include "GameLogic/Cosmetics/CosmeticArmor.h"
#include "GameLogic/Pets/MountRules.h"
#include "World/MapInfra/MapManager.h"
#include <algorithm>

namespace GameLogic::Items
{
bool IsCompanionItem(int itemType)
{
    if (itemType < 0 || itemType >= MAX_ITEM || Pets::IsMountItem(itemType)
        || itemType == ITEM_HORN_OF_UNIRIA || itemType == ITEM_HORN_OF_DINORANT)
        return false;
    return Cosmetics::IsCosmeticPet(itemType) || ItemAttribute[itemType].m_byItemSlot == EQUIPMENT_HELPER;
}

bool CanEquipInMainInventory(const ITEM* item)
{
    return item && item->Type >= 0 && item->Type < MAX_ITEM
        && Cosmetics::ArmorPart(item->Type) < 0;
}

bool CanEquipForClass(int itemType, int baseClass, int stepClass)
{
    if (itemType < 0 || itemType >= MAX_ITEM || baseClass < 0 || baseClass >= MAX_CLASS)
        return false;
    const auto& requirements = ItemAttribute[itemType].RequireClass;
    const int requiredStep = requirements[baseClass];
    if (requiredStep > 0)
        return requiredStep <= stepClass;
    const int skinPart = Cosmetics::ArmorPart(itemType);
    if (skinPart >= 0 && std::none_of(std::begin(requirements), std::end(requirements), [](auto value) { return value > 0; }))
        return true;
    // The stock MG fallback applies to combat gear, never to an explicitly restricted skin.
    return skinPart < 0 && baseClass == CLASS_DARK
        && requirements[CLASS_WIZARD] > 0 && requirements[CLASS_KNIGHT] > 0;
}

bool RequiresFlight(int world)
{
    return world == WD_10HEAVEN;
}

bool CanTakeOff(int equipmentSlot, int world, const ITEM* pItemHelper, const ITEM* pItemWing)
{
    if (!RequiresFlight(world) || !HasFlightEquipment(pItemHelper, pItemWing))
        return true;

    ITEM empty{};
    empty.Type = -1;
    const ITEM* remainingHelper = equipmentSlot == EQUIPMENT_HELPER ? &empty : pItemHelper;
    const ITEM* remainingWing = equipmentSlot == EQUIPMENT_WING ? &empty : pItemWing;
    return HasFlightEquipment(remainingHelper, remainingWing);
}
} // namespace GameLogic::Items

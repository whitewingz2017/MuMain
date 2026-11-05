#include "stdafx.h"
#include "GameLogic/Events/BloodCastleQuestItem.h"
#include "GameLogic/Events/EventProgress.h"
#include "Engine/Object/ZzzCharacter.h"
#include "UI/Core/WindowSystem.h"
#include "UI/Inventory/MyInventory.h"

namespace GameLogic::Events
{
namespace
{
constexpr int ArchangelInventoryType(int itemLevel) { return itemLevel >= 0 && itemLevel <= 2 ? itemLevel + 1 : 0; }
}

void SetArchangelCarrier(int key, int itemType)
{
    for (int i = 0; i < MAX_CHARACTERS_CLIENT; ++i)
    {
        auto& character = CharactersClient[i];
        if (IsArchangelCarrierType(character.EtcPart)) character.EtcPart = 0;
        if (character.Object.Live && character.Key == key && IsArchangelCarrierType(itemType))
            character.EtcPart = itemType;
    }
}

int CarriedArchangelType(const CHARACTER* character)
{
    if (!character) return 0;
    if (character == Hero && g_pMyInventory)
    {
        const int slot = g_pMyInventory->FindItemIndex(ITEM_WEAPON_OF_ARCHANGEL);
        if (slot >= 0)
            if (const auto* item = g_pMyInventory->FindItem(slot)) return ArchangelInventoryType(item->Level);
        return 0; // The local inventory also confirms that a returned or dropped weapon is gone.
    }
    return IsArchangelCarrierType(character->EtcPart) ? character->EtcPart : 0;
}
}

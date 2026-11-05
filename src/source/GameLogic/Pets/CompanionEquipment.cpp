#include "stdafx.h"
#include "GameLogic/Pets/CompanionEquipment.h"
#include "GameLogic/Items/EquipmentRestrictions.h"
#include "GameLogic/Pets/w_PetProcess.h"
#include "UI/Inventory/InventoryCtrl.h"
#include "UI/Core/WindowSystem.h"
#include "Engine/Object/ZzzCharacter.h"
#include <unordered_map>

namespace GameLogic::Pets::CompanionEquipment
{
namespace
{
ITEM item = [] { ITEM empty{}; empty.Type = -1; return empty; }();
struct Appearance { std::wstring name; int type; };
std::unordered_map<CHARACTER*, Appearance> appearances;
}
ITEM& Get() { return item; }

void SetAppearance(CHARACTER* character, int type)
{
    if (!character) return;
    const auto previous = appearances.find(character);
    if (previous != appearances.end() && previous->second.name == character->ID && previous->second.type != type)
        ThePetProcess().DeletePet(character, previous->second.type, false, Slot);
    if (!Items::IsCompanionItem(type))
    {
        appearances.erase(character);
        return;
    }
    appearances[character] = {character->ID, type};
    if (character->Object.Live && !ThePetProcess().HasPet(character, type, Slot))
        ThePetProcess().CreatePet(type, MODEL_ITEM + type, character->Object.Position, character, 0, 0, Slot);
}

bool Receive(std::span<const std::uint8_t> data)
{
    if (!g_pNewItemMng) return false;
    ITEM* received = g_pNewItemMng->CreateItem(data);
    if (!received) return false;
    const bool valid = Items::IsCompanionItem(received->Type);
    if (valid)
    {
        item = *received;
        item.lineal_pos = Slot;
        SetAppearance(Hero, item.Durability > 0 ? item.Type : -1);
    }
    g_pNewItemMng->DeleteItem(received);
    return valid;
}

void Clear()
{
    SetAppearance(Hero, -1);
    item = {};
    item.Type = -1;
}

void SetDurability(std::uint8_t durability)
{
    item.Durability = durability;
    SetAppearance(Hero, durability > 0 ? item.Type : -1);
}

void Update()
{
    if (SceneFlag != MAIN_SCENE)
    {
        Clear();
        appearances.clear();
        return;
    }
    for (auto entry = appearances.begin(); entry != appearances.end();)
    {
        CHARACTER* character = entry->first;
        if (entry->second.name != character->ID || !character->Object.Live)
        {
            entry = appearances.erase(entry);
            continue;
        }
        const int type = entry->second.type;
        if (!ThePetProcess().HasPet(character, type, Slot))
            ThePetProcess().CreatePet(type, MODEL_ITEM + type, character->Object.Position, character, 0, 0, Slot);
        ++entry;
    }
}
}

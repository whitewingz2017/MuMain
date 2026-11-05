#include "stdafx.h"
#include "GameLogic/Pets/MuunMounts.h"
#include "GameLogic/Pets/MountRules.h"
#include "Engine/Object/ZzzCharacter.h"
#include "World/MapInfra/MapManager.h"
#include <unordered_map>

namespace GameLogic::Pets::MuunMounts
{
namespace
{
struct Ride
{
    CHARACTER* character;
    std::wstring name;
    int item = -1, helper = -1, level = 0;
    bool riding = false;
};
std::unordered_map<const OBJECT*, Ride> rides;
}
void Update(CHARACTER* character, int mainItem)
{
    auto found = rides.find(&character->Object);
    if (found != rides.end() && found->second.name != character->ID)
    {
        rides.erase(found);
        found = rides.end();
    }
    if (found == rides.end())
    {
        if (!IsMountItem(mainItem)) return;
        found = rides.emplace(&character->Object, Ride{character, character->ID, mainItem, character->Helper.Type, character->Helper.Level}).first;
    }
    auto& ride = found->second;
    ride.item = mainItem;
    ride.riding = IsMountItem(mainItem) && !character->SafeZone && !character->Dead && !gMapManager.InChaosCastle();
    character->Helper.Type = ride.riding ? (mainItem == DarkHorseItem ? MODEL_DARK_HORSE_ITEM : MODEL_HORN_OF_FENRIR) : ride.helper;
    character->Helper.Level = ride.riding ? 0 : ride.level;
    if (!IsMountItem(mainItem)) rides.erase(found);
}
void EquipmentChanged(CHARACTER* character)
{
    const auto found = rides.find(&character->Object);
    if (found == rides.end()) return;
    found->second.helper = character->Helper.Type;
    found->second.level = character->Helper.Level;
    Update(character, found->second.item);
}
bool IsRiding(const OBJECT* owner, int item)
{
    const auto found = rides.find(owner);
    return found != rides.end() && found->second.riding && (item < 0 || item == found->second.item);
}
int MovementHelper(const CHARACTER* character)
{
    // A ridden Muun mount moves at the speed of the model it rides as (Fenrir or Dark Horse);
    // dismounted (safe zone, death) it falls back to the really equipped helper.
    const auto found = rides.find(&character->Object);
    if (found == rides.end() || found->second.name != character->ID) return character->Helper.Type;
    return found->second.riding ? character->Helper.Type : found->second.helper;
}
void Reset()
{
    for (const auto& [owner, ride] : rides)
        if (ride.character->ID == ride.name)
        {
            ride.character->Helper.Type = ride.helper;
            ride.character->Helper.Level = ride.level;
        }
    rides.clear();
}
}

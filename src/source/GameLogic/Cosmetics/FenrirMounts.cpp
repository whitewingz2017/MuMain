#include "stdafx.h"
#include "GameLogic/Cosmetics/FenrirMounts.h"
#include "GameLogic/Cosmetics/CosmeticArmor.h"
#include <unordered_map>

namespace GameLogic::Cosmetics::FenrirMounts
{
namespace
{
std::unordered_map<const OBJECT*, int> equipped;
}
void SetItem(const OBJECT* owner, int itemType)
{
    if (!owner) return;
    if (IsFenrirMount(itemType)) equipped[owner] = itemType;
    else equipped.erase(owner);
}
int Item(const OBJECT* owner)
{
    const auto found = equipped.find(owner);
    return found == equipped.end() ? -1 : found->second;
}
void Clear(const OBJECT* owner) { equipped.erase(owner); }
void Reset() { equipped.clear(); }
}

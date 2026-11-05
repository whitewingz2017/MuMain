#pragma once

#include <cstdint>
#include "GameLogic/Cosmetics/SkinWeaponRules.h"
#include <span>
#include <string>
#include <vector>

namespace GameLogic::Cosmetics::SkinShop
{
struct Bonus
{
    int number;
    std::wstring label;
};

struct Item
{
    int type;
    int part;
    int price;
    int pool;
    bool eligible;
    std::wstring name;
};

inline bool MatchesWeaponFilter(const Item& item, int family)
{
    return item.eligible && (family == 0 || SkinWeaponFamily(item.type) == family);
}

bool Load();
bool Purchase(const Item& item, int bonus);
void Receive(std::span<const std::uint8_t> packet);
void Update();
void Reset();
bool Pending();
unsigned Revision();
int Balance();
const std::wstring& Status();
std::span<const Item> Items();
std::span<const Bonus> Bonuses(const Item& item);
}

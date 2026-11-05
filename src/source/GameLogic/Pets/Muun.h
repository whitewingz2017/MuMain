#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <vector>
#include "GameLogic/Pets/MountRules.h"

struct OBJECT;
namespace GameLogic::Pets::Muun
{
constexpr int FirstType = 13 * 512 + 450;
constexpr int SpeciesCapacity = 30;
constexpr bool IsType(int type)
{
    if (IsMountItem(type)) return false;
    if (type >= FirstType && type < FirstType + SpeciesCapacity) return true;
    if (type == 13 * 512 + 200) return true;
    for (const auto& pet : GameLogic::Cosmetics::FinalMuSkins)
        if (pet.type == type) return pet.part == 8;
    return false;
}
// Mirrors the server's MuunInactiveReason values; they are a wire contract.
enum class InactiveReason : std::uint8_t { None, NotEquipped, NoLife, LevelTooHigh, Expired, CharacterLevel, Map, Day, Hour, Count };
constexpr std::int64_t NeverExpires = -1;
struct Item
{
    int slot = -1, type = -1, level = 0; double life = 0, maximumLife = 0; bool active = false; std::string name;
    InactiveReason inactive = InactiveReason::None;
    std::int64_t expiresInSeconds = NeverExpires;
};
struct Species { int type = -1, maximumLevel = 0, evolution = -1, stone = -1, repair = -1; std::string name, description; bool owned = false; };
struct Exchange { int index = 0, material = -1, count = 1; std::string name; };
struct State
{
    std::vector<Item> items;
    std::vector<Species> species;
    std::vector<Exchange> exchanges;
    std::string status;
    std::uint64_t revision = 0;
    int capacity = 102;
    bool enabled = false, pending = false, supportsSlotMoves = false;
};
const State& Get();
const Species* Rule(int type);
bool Request(std::uint8_t operation = 0, int source = 0, int target = 0);
void Receive(std::span<const std::uint8_t> packet);
void ReceiveAppearance(std::span<const std::uint8_t> packet);
void Update();
void Reset();
float FollowSide(const OBJECT* owner, int type);
}

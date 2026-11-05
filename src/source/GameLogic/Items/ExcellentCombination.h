#pragma once
#include <cstdint>
#include <span>
#include <string>
typedef struct tagITEM ITEM;
namespace GameLogic::Items::ExcellentCombination {
constexpr int ResonanceCoreType = 14 * 512 + 252;
constexpr int MaximumCoreStack = 255;
bool CanStackCores(const ITEM* source, const ITEM* target);
struct State { bool pending=false, valid=false, success=false, completed=false, configured=false; std::uint64_t revision=0; std::string status; int cost=1000000; int excellentFlags=0; int baseRate=35, bonusPerCore=10, maximumRate=75, availableCores=0, selectedCores=0; };
const State& Get();
bool IsEligible(const ITEM* item);
bool Request(bool combine, int primary, int secondary, int cores=0);
bool RequestConfiguration();
void Receive(std::span<const std::uint8_t> packet);
void Update();
void Clear();
}

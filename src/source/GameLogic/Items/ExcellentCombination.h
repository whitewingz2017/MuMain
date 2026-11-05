#pragma once
#include <cstdint>
#include <span>
#include <string>
namespace GameLogic::Items::ExcellentCombination {
struct State { bool pending=false, valid=false, success=false; std::uint64_t revision=0; std::string status; int cost=1000000; int excellentFlags=0; };
const State& Get();
bool Request(bool combine, int primary, int secondary);
void Receive(std::span<const std::uint8_t> packet);
void Update();
void Clear();
}

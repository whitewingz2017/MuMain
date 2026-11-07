#pragma once

#include <cstdint>
#include <span>

class CHARACTER;

// The rank each player in view has, as the server sends it (C2 FD 07, docs/rank-titles.md).
namespace GameLogic::RankTitles
{
inline constexpr std::uint8_t PacketHeadCode = 0xFD;
inline constexpr std::uint8_t PacketSubCode = 0x07;

void Receive(std::span<const std::uint8_t> packet);

// The character's rank, 0 when it has none.
int RankOf(const CHARACTER* character);

void SetOwnRank(int rank);

void Reset();
} // namespace GameLogic::RankTitles

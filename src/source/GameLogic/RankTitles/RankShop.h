#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>

// Buying rank titles for WCoinC and choosing the one shown (C1/C2 FD 08, docs/rank-titles.md).
// The server owns prices, the balance and the titles a character owns; every answer carries all of it.
namespace GameLogic::RankTitles::Shop
{
inline constexpr std::uint8_t PacketSubCode = 0x08;

enum class Operation : std::uint8_t
{
    Load = 0,
    Buy = 1,
    Equip = 2,
};

struct Offer
{
    int rank = 0;
    int price = 0;
    bool owned = false;
    std::wstring name;
};

bool Load();
// Buys at the price the shop showed; the server refuses it when the price changed.
bool Buy(const Offer& offer);
// Shows an owned title; rank 0 shows none.
bool Equip(int rank);

void Receive(std::span<const std::uint8_t> packet);
// Times out a request the server never answered, and forgets the shop when the character changes.
void Update();
void Reset();

bool Pending();
// Counts the answers, so a window rebuilds only after one.
unsigned Revision();
std::optional<int> Balance();
// The title the character shows, 0 for none.
int Active();
const std::wstring& Status();
std::span<const Offer> Offers();
} // namespace GameLogic::RankTitles::Shop

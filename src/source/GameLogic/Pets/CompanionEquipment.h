#pragma once
#include <cstdint>
#include <span>
typedef struct tagITEM ITEM;
class CHARACTER;

namespace GameLogic::Pets::CompanionEquipment
{
// Keep the former Skin pet position so existing owned pets retain their saved slot.
inline constexpr int Slot = 248;
inline constexpr int Part = 8;
ITEM& Get();
bool Receive(std::span<const std::uint8_t> data);
void Clear();
void SetDurability(std::uint8_t durability);
void SetAppearance(CHARACTER* character, int type);
void Update();
}

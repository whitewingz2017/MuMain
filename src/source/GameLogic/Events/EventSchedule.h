#pragma once
#include <cstdint>
#include <span>

namespace GameLogic::Events::Schedule
{
void Open();
void Update();
void Receive(std::span<const std::uint8_t> packet);
}

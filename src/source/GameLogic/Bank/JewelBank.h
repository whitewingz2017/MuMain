#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <vector>
namespace GameLogic::Bank
{
struct JewelRow { int item = 0; std::string name; int balance = 0, inventory = 0, limit = 0; };
struct Snapshot { std::uint32_t requestId = 0; std::string status; std::vector<JewelRow> rows; };
bool ParseSnapshot(std::span<const std::uint8_t> packet, Snapshot& snapshot);
bool Request(std::uint8_t operation, std::uint16_t item = 0, std::uint32_t amount = 0);
void Receive(std::span<const std::uint8_t> packet);
}

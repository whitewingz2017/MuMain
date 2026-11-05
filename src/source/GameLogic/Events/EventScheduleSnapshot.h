#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <vector>
#include "json.hpp"

namespace GameLogic::Events::Schedule
{
struct Row { std::string name; int seconds; bool active; };
struct Snapshot { std::uint32_t requestId; std::vector<Row> rows; };
inline bool Parse(std::span<const std::uint8_t> packet, Snapshot& result)
{
    constexpr int HeaderSize = 5, MaxRows = 128, MaxSeconds = 7 * 24 * 3600;
    if (packet.size() < HeaderSize || packet[0] != 0xC2 || packet[3] != 0xFB || packet[4] != 0x21 ||
        ((packet[1] << 8) | packet[2]) != packet.size()) return false;
    const auto json = nlohmann::json::parse(packet.begin() + HeaderSize, packet.end(), nullptr, false);
    if (!json.is_object() || !json.contains("RequestId") || !json["RequestId"].is_number_unsigned()) return false;
    const auto id = json["RequestId"].get<std::uint64_t>();
    if (!id || id > UINT32_MAX) return false;
    Snapshot next{static_cast<std::uint32_t>(id), {}};
    for (const char* group : {"Active", "Rows"})
    {
        if (!json.contains(group) || !json[group].is_array() || json[group].size() > MaxRows) return false;
        for (const auto& row : json[group])
        {
            if (!row.is_object() || !row.contains("Name") || !row["Name"].is_string() ||
                !row.contains("Seconds") || !row["Seconds"].is_number_integer()) return false;
            const auto name = row["Name"].get<std::string>();
            const auto seconds = row["Seconds"].get<std::int64_t>();
            if (name.empty() || name.size() > 256 || seconds < 0 || seconds > MaxSeconds) return false;
            next.rows.push_back({name, static_cast<int>(seconds), group[0] == 'A'});
        }
    }
    result = std::move(next);
    return true;
}
}

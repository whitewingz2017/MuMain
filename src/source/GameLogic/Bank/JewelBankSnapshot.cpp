#include "GameLogic/Bank/JewelBank.h"
#include "json.hpp"
namespace GameLogic::Bank
{
bool ParseSnapshot(std::span<const std::uint8_t> packet, Snapshot& snapshot)
{
    if (packet.size() < 7 || packet.size() > 32768 || packet[0] != 0xC2 || packet[3] != 0xFA || packet[4] != 0x19 ||
        ((packet[1] << 8) | packet[2]) != packet.size()) return false;
    try
    {
        const auto json = nlohmann::json::parse(packet.begin() + 5, packet.end());
        Snapshot result;
        const auto& request = json.at("RequestId");
        if (!request.is_number_integer() || request.get<std::int64_t>() < 0 || request.get<std::uint64_t>() > UINT32_MAX) return false;
        result.requestId = request.get<std::uint32_t>();
        result.status = json.at("Status").get<std::string>();
        const auto& rows = json.at("Rows");
        if (!rows.is_array() || rows.size() > 32 || result.status.size() > 512) return false;
        for (const auto& entry : rows)
        {
            for (const auto* key : {"Item", "Balance", "Inventory", "Limit"})
                if (!entry.at(key).is_number_integer() || entry.at(key).get<std::int64_t>() < 0 || entry.at(key).get<std::uint64_t>() > INT32_MAX) return false;
            JewelRow row{entry.at("Item").get<int>(), entry.at("Name").get<std::string>(),
                entry.at("Balance").get<int>(), entry.at("Inventory").get<int>(), entry.at("Limit").get<int>()};
            if (row.item < 0 || row.item > 8191 || row.balance < 0 || row.inventory < 0 || row.limit <= 0 || row.name.size() > 128) return false;
            result.rows.push_back(std::move(row));
        }
        snapshot = std::move(result);
        return true;
    }
    catch (const nlohmann::json::exception&) { return false; }
}
}

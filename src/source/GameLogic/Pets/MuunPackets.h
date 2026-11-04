#pragma once
#include "GameLogic/Pets/Muun.h"
#include "json.hpp"
#include <array>
#include <cmath>
namespace GameLogic::Pets::Muun::Wire {
constexpr int HeaderSize = 5, MaximumCollection = 120;
inline nlohmann::json Payload(std::span<const std::uint8_t> packet, int subcode)
{
    if (packet.size() <= HeaderSize || packet[0] != 0xC2 || packet[3] != 0xFD || packet[4] != subcode
        || ((packet[1] << 8) | packet[2]) != packet.size()) return {};
    return nlohmann::json::parse(packet.begin() + HeaderSize, packet.end(), nullptr, false);
}
inline bool Integer(const nlohmann::json& object, const char* key, int minimum, int maximum)
{
    const auto i = object.find(key);
    return i != object.end() && i->is_number_integer() && *i >= minimum && *i <= maximum;
}
// Older servers and the oversized-catalogue fallback omit these fields.
inline bool OptionalDetails(const nlohmann::json& item, Item& target)
{
    constexpr int LastReason = static_cast<int>(InactiveReason::Count) - 1;
    constexpr std::int64_t MaximumExpirySeconds = 10LL * 365 * 24 * 60 * 60;
    if (item.contains("Inactive"))
    {
        if (!Integer(item, "Inactive", 0, LastReason)) return false;
        target.inactive = static_cast<InactiveReason>(item["Inactive"].get<int>());
    }
    if (item.contains("ExpiresInSeconds"))
    {
        const auto& expiry = item["ExpiresInSeconds"];
        if (!expiry.is_number_integer() || expiry < NeverExpires || expiry > MaximumExpirySeconds) return false;
        target.expiresInSeconds = expiry.get<std::int64_t>();
    }
    return true;
}
inline bool Snapshot(std::span<const std::uint8_t> packet, std::uint32_t request, State& next)
{
    const auto json = Payload(packet, 5);
    if (!json.is_object() || !json.contains("RequestId") || !json["RequestId"].is_number_unsigned() || json["RequestId"] != request
        || !json.contains("Enabled") || !json["Enabled"].is_boolean() || !json.contains("Status") || !json["Status"].is_string()
        || json["Status"].get_ref<const std::string&>().size() > 1024) return false;
    next = {};
    next.enabled = json["Enabled"].get<bool>(); next.status = json["Status"].get<std::string>();
    if (json.contains("SupportsSlotMoves") && !json["SupportsSlotMoves"].is_boolean()) return false;
    next.supportsSlotMoves = json.value("SupportsSlotMoves", false);
    if (next.enabled)
    {
        if (!Integer(json, "Capacity", 3, MaximumCollection)) return false;
        next.capacity = json["Capacity"];
        if (!json.contains("Species") || !json["Species"].is_array() || json["Species"].size() > MaximumCollection) return false;
        for (const auto& rule : json["Species"])
        {
            if (!rule.is_object() || !Integer(rule, "Type", 0, 8191) || !Integer(rule, "MaximumLevel", 0, 15)
                || !Integer(rule, "EvolutionItemType", -1, 8191) || !Integer(rule, "EvolutionStoneType", 0, 8191)
                || !Integer(rule, "RepairItemType", 0, 8191)) return false;
            next.species.push_back({rule["Type"], rule["MaximumLevel"], rule["EvolutionItemType"], rule["EvolutionStoneType"], rule["RepairItemType"]});
            auto& species = next.species.back();
            for (const auto* key : {"Name", "Description"})
                if (rule.contains(key) && (!rule[key].is_string() || rule[key].get_ref<const std::string&>().size() > 1024)) return false;
            if (rule.contains("Owned") && !rule["Owned"].is_boolean()) return false;
            species.name = rule.value("Name", std::string{}); species.description = rule.value("Description", std::string{});
            species.owned = rule.value("Owned", false);
        }
        if (json.contains("Exchanges") && !json["Exchanges"].is_null()) {
            if (!json["Exchanges"].is_array() || json["Exchanges"].size() > 64) return false;
            std::array<bool, 64> recipes{};
            for (const auto& recipe : json["Exchanges"]) {
                if (!recipe.is_object() || !Integer(recipe, "Index", 0, 63) || !Integer(recipe, "MaterialItemType", 0, 8191)
                    || !Integer(recipe, "RequiredCount", 1, 255) || !recipe.contains("Name") || !recipe["Name"].is_string()
                    || recipe["Name"].get_ref<const std::string&>().size() > 512) return false;
                const int index = recipe["Index"]; if (recipes[index]) return false; recipes[index] = true;
                next.exchanges.push_back({index, recipe["MaterialItemType"], recipe["RequiredCount"], recipe["Name"]});
            }
        }
        if (json.contains("Items") && !json["Items"].is_null())
        {
            if (!json["Items"].is_array() || json["Items"].size() > MaximumCollection) return false;
            std::array<bool, MaximumCollection> slots{};
            for (const auto& item : json["Items"])
            {
                if (!item.is_object() || !Integer(item, "Slot", 0, next.capacity - 1) || !Integer(item, "Type", 0, 8191)
                    || !Integer(item, "Level", 0, 15) || !item.contains("Life") || !item["Life"].is_number()
                    || !item.contains("MaximumLife") || !item["MaximumLife"].is_number()
                    || !item.contains("Active") || !item["Active"].is_boolean() || !item.contains("Name") || !item["Name"].is_string()
                    || item["Name"].get_ref<const std::string&>().size() > 256) return false;
                const int slot = item["Slot"];
                const double life = item["Life"], maximum = item["MaximumLife"];
                if (slots[slot] || !std::isfinite(life) || !std::isfinite(maximum) || life < 0 || maximum < 0 || life > maximum) return false;
                slots[slot] = true;
                next.items.push_back({slot, item["Type"], item["Level"], life, maximum, item["Active"], item["Name"]});
                if (!OptionalDetails(item, next.items.back())) return false;
            }
        }
    }
    return true;
}
}

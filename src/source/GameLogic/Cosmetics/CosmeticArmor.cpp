#include "stdafx.h"
#include "GameLogic/Cosmetics/CosmeticArmor.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Network/Server/SocketSystem.h"
#include "UI/Chat/ChatMessages.h"
#include "Core/Utilities/StringUtils.h"
#include "Dotnet/Connection.h"
#include "Network/Server/WSclient.h"
#include "GameLogic/Pets/CompanionEquipment.h"
#include "GameLogic/Items/EquipmentRestrictions.h"
#include <SDL3/SDL.h>

#include "json.hpp"
#include <array>
#include <vector>
#include <cstdio>

namespace GameLogic::Cosmetics
{
namespace
{
constexpr int RequestSize = 11;
constexpr int ResponseHeaderSize = 5;
constexpr Uint64 RequestTimeoutMs = 8000;
std::array<int, SkinSlotCount> armor = {-1, -1, -1, -1, -1, -1, -1, -1, -1};
std::array<int, SkinSlotCount> flags{};
std::array<int, SkinSlotCount> levels{};
std::wstring owner;
std::uint32_t sequence = 0;
std::uint32_t pending = 0;
Uint64 sentAt = 0;
bool ready = false;
bool storageSupported = false;
std::vector<StoredSkin> storedSkins;
std::uint64_t revision = 0;
std::vector<std::string> statistics;

std::vector<std::string> ReadStatistics(const nlohmann::json& json)
{
    std::vector<std::string> confirmedStatistics;
    if (json.contains("Statistics") && json["Statistics"].is_object() && json["Statistics"].size() <= 32)
    {
        for (const auto& [label, value] : json["Statistics"].items())
        {
            if (label.size() > 80 || !value.is_number()) continue;
            const auto number = value.get<double>();
            if (!std::isfinite(number)) continue;
            char text[128];
            std::snprintf(text, sizeof(text), "%s: %.2f", label.c_str(), number);
            confirmedStatistics.emplace_back(text);
        }
    }
    return confirmedStatistics;
}


}

void Reset()
{
    armor.fill(-1);
    flags.fill(0);
    levels.fill(0);
    statistics.clear();
    storedSkins.clear();
    storageSupported = false;
    ++revision;
    owner.clear();
    pending = 0;
    ready = false;
}

bool Request(std::uint8_t operation, int inventorySlot, int part)
{
    if (!SocketClient || !SocketClient->IsConnected() || SceneFlag != MAIN_SCENE || !Hero
        || operation > 4 || inventorySlot < 0 || inventorySlot > 255 || part < 0 || part >= SkinSlotCount)
        return false;
    if (operation >= 3 && !StorageSupported()) return false;
    if (pending && SDL_GetTicks() - sentAt < RequestTimeoutMs)
        return false;
    if (owner != Hero->ID)
    {
        Reset();
        owner = Hero->ID;
    }
    pending = ++sequence;
    if (!pending) pending = ++sequence;
    sentAt = SDL_GetTicks();
    BYTE packet[RequestSize] = {0xC1, RequestSize, 0xFD, 0x01, operation,
        static_cast<BYTE>(inventorySlot), static_cast<BYTE>(part)};
    for (int byte = 0; byte < 4; ++byte)
        packet[7 + byte] = (pending >> (8 * byte)) & 255;
    SocketClient->Send(packet, sizeof(packet));
    return true;
}

void Update()
{
    GameLogic::Pets::CompanionEquipment::Update();
    if (SceneFlag != MAIN_SCENE || !Hero)
    {
        Reset();
        return;
    }
    if (owner != Hero->ID)
    {
        Request(0);
        return;
    }
}

void Receive(std::span<const std::uint8_t> packet)
{
    if (packet.size() <= ResponseHeaderSize || packet[0] != 0xC2 || packet[3] != 0xFD || packet[4] != 1
        || ((packet[1] << 8) | packet[2]) != packet.size()
        || !pending || SceneFlag != MAIN_SCENE || !Hero || owner != Hero->ID)
        return;
    const auto json = nlohmann::json::parse(packet.begin() + ResponseHeaderSize, packet.end(), nullptr, false);
    if (json.is_discarded() || !json.is_object() || !json.contains("RequestId")
        || !json["RequestId"].is_number_unsigned() || json["RequestId"] != pending
        || !json.contains("Skins") || !json["Skins"].is_array() || json["Skins"].size() > SkinSlotCount)
        return;
    std::array<int, SkinSlotCount> confirmed = {-1, -1, -1, -1, -1, -1, -1, -1, -1};
    std::array<int, SkinSlotCount> confirmedFlags{};
    std::array<int, SkinSlotCount> confirmedLevels{};
    for (const auto& item : json["Skins"])
    {
        if (!item.is_object() || !item.contains("Part") || !item["Part"].is_number_integer()
            || !item.contains("ItemType") || !item["ItemType"].is_number_integer()) return;
        const auto part = item["Part"].get<std::int64_t>();
        const auto type = item["ItemType"].get<std::int64_t>();
        if (part < 0 || part >= SkinSlotCount || type < 0 || type > INT_MAX || confirmed[part] != -1) return;
        // A skin this client has no model for (the server got new skins first) is shown as no
        // skin; rejecting the whole answer dropped every skin and the skin pages with it.
        if (!FitsPart(static_cast<int>(type), static_cast<int>(part))
            && !(part == GameLogic::Pets::CompanionEquipment::Part && GameLogic::Items::IsCompanionItem(static_cast<int>(type))))
            continue;
        confirmed[part] = static_cast<int>(type);
        if (item.contains("BonusFlags"))
        {
            if (!item["BonusFlags"].is_number_integer()) return;
            const auto bonus = item["BonusFlags"].get<std::int64_t>();
            if (bonus < 0 || bonus > 63) return;
            confirmedFlags[part] = static_cast<int>(bonus);
        }
        if (item.contains("Level"))
        {
            if (!item["Level"].is_number_integer()) return;
            const auto level = item["Level"].get<std::int64_t>();
            if (level < 0 || level > 15) return;
            confirmedLevels[part] = static_cast<int>(level);
        }
    }
    std::vector<StoredSkin> stored;
    const bool hasStorage = json.contains("StoredSkins");
    if (hasStorage && !json["StoredSkins"].is_null())
    {
        const auto& entries = json["StoredSkins"];
        if (!entries.is_array() || entries.size() > 120) return;
        std::array<bool, 120> occupied{};
        for (const auto& entry : entries)
        {
            if (!entry.is_object()) return;
            for (const char* field : {"Slot", "ItemType", "Level", "BonusFlags"})
                if (!entry.contains(field) || !entry[field].is_number_integer()) return;
            const auto slot = entry["Slot"].get<std::int64_t>();
            const auto type = entry["ItemType"].get<std::int64_t>();
            const auto level = entry["Level"].get<std::int64_t>();
            const auto bonus = entry["BonusFlags"].get<std::int64_t>();
            if (slot < 0 || slot >= occupied.size() || occupied[slot] || type < 0 || type >= MAX_ITEM
                || level < 0 || level > 15 || bonus < 0 || bonus > 63) return;
            occupied[slot] = true;
            // Unknown to this client (see above): its slot stays taken, the skin is not listed.
            if (ArmorPart(static_cast<int>(type)) < 0 && !IsFenrirMount(static_cast<int>(type)))
                continue;
            stored.push_back({static_cast<int>(slot), static_cast<int>(type), static_cast<int>(level), static_cast<int>(bonus)});
        }
    }
    storedSkins = std::move(stored);
    storageSupported = hasStorage;
    ++revision;
    statistics = ReadStatistics(json);
    armor = confirmed;
    flags = confirmedFlags;
    levels = confirmedLevels;
    ready = true;
    pending = 0;
    if (json.contains("Status") && json["Status"].is_string() && json["Status"] != "Ready")
    {
        const auto text = json["Status"].get<std::string>();
        UI::Chat::PostSystem(StringUtils::NarrowToWide(text), mu::ui::window::TYPE_SYSTEM_MESSAGE);
    }
}

int EquippedArmor(int part)
{
    return Ready() && part >= 0 && part < SkinSlotCount ? armor[part] : -1;
}

int EquippedFlags(int part)
{
    return Ready() && part >= 0 && part < SkinSlotCount ? flags[part] : 0;
}

int EquippedLevel(int part)
{
    return Ready() && part >= 0 && part < SkinSlotCount ? levels[part] : 0;
}

int VisualLevel(int part)
{
    // The armor parts show the full set equipped in part 1, as VisualItem does.
    return EquippedLevel(part >= 0 && part < ArmorPartCount ? 1 : part);
}

int VisualItem(int part)
{
    if (!Ready()) return -1;
    if (part >= 0 && part < ArmorPartCount)
    {
        const int set = EquippedArmor(1);
        return FullSetVisualItem(set, part);
    }
    return EquippedArmor(part);
}

std::span<const std::string> Statistics()
{
    return statistics;
}

std::span<const StoredSkin> StoredSkins() { return storedSkins; }
bool StorageSupported() { return Ready() && storageSupported; }
bool Pending() { return pending && SDL_GetTicks() - sentAt < RequestTimeoutMs; }
std::uint64_t Revision() { return revision; }

bool Ready()
{
    return ready && SceneFlag == MAIN_SCENE && Hero && owner == Hero->ID;
}
}


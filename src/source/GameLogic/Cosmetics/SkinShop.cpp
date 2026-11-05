#include "stdafx.h"
#include "GameLogic/Cosmetics/SkinShop.h"
#include "GameLogic/Cosmetics/CosmeticArmor.h"
#include "Core/Utilities/StringUtils.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Network/Server/SocketSystem.h"
#include "Dotnet/Connection.h"
#include "Network/Server/WSclient.h"
#include "json.hpp"
#include <SDL3/SDL.h>
#include <map>

namespace GameLogic::Cosmetics::SkinShop
{
namespace
{
// The level byte follows the 16-byte request the server read before it took a level.
constexpr int RequestSize = 17;
constexpr int MaxLevel = 15;
constexpr int HeaderSize = 5;
constexpr int MaxCatalogItems = 1024;
constexpr int MaxBonusChoices = 6;
constexpr int MaxPools = 16;
constexpr int MaxLabelBytes = 256;
constexpr Uint64 TimeoutMs = 8000;
std::vector<Item> items;
std::map<int, std::vector<Bonus>> pools;
std::wstring status;
std::wstring owner;
std::uint32_t sequence = 0;
std::uint32_t pending = 0;
unsigned revision = 0;
int balance = 0;
bool catalogRequest = false;
Uint64 sentAt = 0;

bool Integer(const nlohmann::json& object, const char* key, int low, int high, int& result)
{
    const auto entry = object.find(key);
    if (entry == object.end() || !entry->is_number_integer()) return false;
    const auto value = entry->get<std::int64_t>();
    if (value < low || value > high) return false;
    result = static_cast<int>(value);
    return true;
}

bool Label(const nlohmann::json& object, const char* key, std::wstring& result)
{
    const auto entry = object.find(key);
    if (entry == object.end() || !entry->is_string()) return false;
    const auto& value = entry->get_ref<const std::string&>();
    if (value.size() > MaxLabelBytes) return false;
    result = StringUtils::NarrowToWide(value);
    return true;
}

bool ReadCatalog(const nlohmann::json& json, std::vector<Item>& catalog, std::map<int, std::vector<Bonus>>& choices)
{
    if (!json.contains("Items") || !json["Items"].is_array() || json["Items"].size() > MaxCatalogItems
        || !json.contains("Pools") || !json["Pools"].is_array() || json["Pools"].size() > MaxPools)
        return false;
    for (const auto& group : json["Pools"])
    {
        int pool;
        if (!group.is_object() || !Integer(group, "Pool", 0, 15, pool) || choices.contains(pool)
            || !group.contains("Bonuses") || !group["Bonuses"].is_array() || group["Bonuses"].size() > MaxBonusChoices)
            return false;
        auto& entries = choices[pool];
        for (const auto& value : group["Bonuses"])
        {
            Bonus bonus{};
            if (!value.is_object() || !Integer(value, "Number", 1, MaxBonusChoices, bonus.number) || !Label(value, "Label", bonus.label)
                || std::any_of(entries.begin(), entries.end(), [&](const Bonus& entry) { return entry.number == bonus.number; }))
                return false;
            entries.push_back(std::move(bonus));
        }
    }
    for (const auto& value : json["Items"])
    {
        Item item{};
        if (!value.is_object() || !Integer(value, "ItemType", 0, 16 * 512 - 1, item.type)
            || !Integer(value, "Part", 0, SkinSlotCount - 1, item.part)
            || !Integer(value, "Price", 0, INT_MAX, item.price) || !Integer(value, "Pool", 0, 15, item.pool)
            || !choices.contains(item.pool) || !Label(value, "Name", item.name)
            || !value.contains("Eligible") || !value["Eligible"].is_boolean()
            || std::any_of(catalog.begin(), catalog.end(), [&](const Item& entry) { return entry.type == item.type; }))
            return false;
        // A skin this client has no model for (the server got new skins first) is left out of the
        // shop; rejecting the whole catalog for it emptied the shop of every skin.
        if (IsFenrirMount(item.type) ? item.part != 8 : ArmorPart(item.type) != item.part)
            continue;
        item.eligible = value["Eligible"].get<bool>();
        catalog.push_back(std::move(item));
    }
    return true;
}

bool Send(bool load, int type, int bonus, int price, int level)
{
    if (pending || !Ready() || !SocketClient || !SocketClient->IsConnected()) return false;
    pending = ++sequence;
    if (!pending) pending = ++sequence;
    owner = Hero->ID;
    sentAt = SDL_GetTicks();
    catalogRequest = load;
    BYTE packet[RequestSize] = {0xC1, RequestSize, 0xFD, 0x02, static_cast<BYTE>(load ? 0 : 1),
        static_cast<BYTE>(type), static_cast<BYTE>(type >> 8), static_cast<BYTE>(bonus)};
    for (int byte = 0; byte < 4; ++byte)
    {
        packet[8 + byte] = (price >> (8 * byte)) & 255;
        packet[12 + byte] = (pending >> (8 * byte)) & 255;
    }
    packet[16] = static_cast<BYTE>(level);
    SocketClient->Send(packet, sizeof(packet));
    return true;
}
}

bool Load() { return Send(true, 0, 0, 0, 0); }

bool Purchase(const Item& item, int bonus, int level)
{
    if (item.price <= 0 || !item.eligible || item.price > balance || level < 0 || level > MaxLevel) return false;
    const auto choices = Bonuses(item);
    if (choices.empty() ? bonus != 0 : std::none_of(choices.begin(), choices.end(),
        [bonus](const Bonus& entry) { return entry.number == bonus; })) return false;
    return Send(false, item.type, bonus, item.price, level);
}

void Receive(std::span<const std::uint8_t> packet)
{
    if (!pending || !Ready() || owner != Hero->ID || packet.size() <= HeaderSize || packet[0] != 0xC2
        || packet[3] != 0xFD || packet[4] != 2 || ((packet[1] << 8) | packet[2]) != packet.size()) return;
    const auto json = nlohmann::json::parse(packet.begin() + HeaderSize, packet.end(), nullptr, false);
    int confirmedBalance;
    std::wstring confirmedStatus;
    if (!json.is_object() || !json.contains("RequestId") || !json["RequestId"].is_number_unsigned()
        || json["RequestId"] != pending || !Integer(json, "Balance", 0, INT_MAX, confirmedBalance)
        || !Label(json, "Status", confirmedStatus)) return;
    if (catalogRequest)
    {
        std::vector<Item> catalog;
        std::map<int, std::vector<Bonus>> choices;
        if (!ReadCatalog(json, catalog, choices)) return;
        items = std::move(catalog);
        pools = std::move(choices);
    }
    balance = confirmedBalance;
    status = std::move(confirmedStatus);
    pending = 0;
    ++revision;
}

void Reset()
{
    pending = 0;
    owner.clear();
    items.clear();
    pools.clear();
    balance = 0;
    status.clear();
}

void Update()
{
    if (!Ready() || !SocketClient || !SocketClient->IsConnected() || (!owner.empty() && owner != Hero->ID))
    {
        Reset();
        return;
    }
    if (pending && SDL_GetTicks() - sentAt >= TimeoutMs)
    {
        pending = 0;
        status = L"Shop request timed out. Refresh the shop and check your inventory before buying again.";
        ++revision;
    }
}

bool Pending() { return pending != 0; }
unsigned Revision() { return revision; }
int Balance() { return balance; }
const std::wstring& Status() { return status; }
std::span<const Item> Items() { return items; }
std::span<const Bonus> Bonuses(const Item& item)
{
    const auto found = pools.find(item.pool);
    return found == pools.end() ? std::span<const Bonus>{} : found->second;
}
}

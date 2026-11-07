#include "stdafx.h"
#include "GameLogic/RankTitles/RankTitleStore.h"

#include "Core/Utilities/StringUtils.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Scenes/SceneCore.h"

#include "json.hpp"

#include <string>
#include <unordered_map>

namespace GameLogic::RankTitles
{
namespace
{
using Json = nlohmann::json;

constexpr size_t HeaderSize = 5;
constexpr int MaximumId = 32767;
constexpr int MaximumRank = 65535;
constexpr size_t MaximumNameLength = 40;

struct Entry
{
    std::wstring name;
    int rank = 0;
};

// By the character's id; the name tells an entry from a later character that got the same id.
std::unordered_map<int, Entry> ranks;
std::wstring owner;

bool IsRankPacket(std::span<const std::uint8_t> packet)
{
    return packet.size() > HeaderSize && packet[0] == 0xC2 && packet[3] == PacketHeadCode
        && packet[4] == PacketSubCode && ((packet[1] << 8) | packet[2]) == static_cast<int>(packet.size());
}

bool IsInteger(const Json& object, const char* key, int minimum, int maximum)
{
    const auto value = object.find(key);
    return value != object.end() && value->is_number_integer() && *value >= minimum && *value <= maximum;
}

bool IsCurrent(int id, const Entry& entry)
{
    const int index = FindCharacterIndex(id);
    return index >= 0 && index < MAX_CHARACTERS_CLIENT && entry.name == CharactersClient[index].ID;
}

// Entries of characters that left the view, so the map stays as small as the view.
void DropStale()
{
    std::erase_if(ranks, [](const auto& item) { return !IsCurrent(item.first, item.second); });
}

void Store(int id, std::wstring name, int rank)
{
    if (rank == 0)
    {
        ranks.erase(id);
        return;
    }
    if (!ranks.contains(id) && ranks.size() >= MAX_CHARACTERS_CLIENT)
        DropStale();
    if (!ranks.contains(id) && ranks.size() >= MAX_CHARACTERS_CLIENT)
        return;
    ranks[id] = Entry{std::move(name), rank};
}

void ReadEntry(const Json& entry)
{
    if (!entry.is_object() || !IsInteger(entry, "Id", 0, MaximumId) || !IsInteger(entry, "Rank", 0, MaximumRank))
        return;
    const auto name = entry.find("Name");
    if (name == entry.end() || !name->is_string())
        return;
    const auto& text = name->get_ref<const std::string&>();
    if (text.empty() || text.size() > MaximumNameLength)
        return;
    Store(entry["Id"].get<int>(), StringUtils::NarrowToWide(text), entry["Rank"].get<int>());
}

bool StartSession()
{
    if (SceneFlag != MAIN_SCENE || !Hero)
        return false;
    if (owner != Hero->ID)
    {
        Reset();
        owner = Hero->ID;
    }
    return true;
}
} // namespace

void Receive(std::span<const std::uint8_t> packet)
{
    if (!IsRankPacket(packet) || !StartSession())
        return;
    const Json json = Json::parse(packet.begin() + HeaderSize, packet.end(), nullptr, false);
    if (json.is_discarded() || !json.is_object())
        return;
    const auto entries = json.find("Ranks");
    if (entries == json.end() || !entries->is_array() || entries->size() > MAX_CHARACTERS_CLIENT)
        return;
    for (const Json& entry : *entries)
        ReadEntry(entry);
}

int RankOf(const CHARACTER* character)
{
    if (!character || ranks.empty())
        return 0;
    const auto found = ranks.find(character->Key);
    if (found == ranks.end() || found->second.name != character->ID)
        return 0;
    return found->second.rank;
}

void SetOwnRank(int rank)
{
    if (rank < 0 || rank > MaximumRank || !StartSession())
        return;
    Store(Hero->Key, Hero->ID, rank);
}

void Reset()
{
    ranks.clear();
    owner.clear();
}
} // namespace GameLogic::RankTitles

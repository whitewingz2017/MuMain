#include "stdafx.h"
#include "GameLogic/RankTitles/RankShop.h"

#include "Core/Utilities/StringUtils.h"
#include "Data/GameData/RankTitles/RankTitles.h"
#include "Dotnet/Connection.h"
#include "Engine/Object/ZzzCharacter.h"
#include "GameLogic/RankTitles/RankTitleStore.h"
#include "Network/Server/WSclient.h"
#include "Scenes/SceneCore.h"

#include "json.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <climits>
#include <vector>

namespace GameLogic::RankTitles::Shop
{
namespace
{
using Json = nlohmann::json;

// C1 size FD 08, operation, rank (2), price (4), request id (4).
constexpr int RequestSize = 15;
constexpr int PriceOffset = 7;
constexpr int RequestIdOffset = 11;
constexpr size_t HeaderSize = 5;
constexpr int MaximumRank = 65535;
constexpr size_t MaximumOffers = 1024;
constexpr size_t MaximumTextBytes = 256;
constexpr Uint64 TimeoutMs = 8000;

std::vector<Offer> offers;
std::wstring status;
std::wstring owner;
std::uint32_t sequence = 0;
std::uint32_t pending = 0;
Uint64 sentAt = 0;
unsigned revision = 0;
std::optional<int> balance;
int active = 0;

bool Ready()
{
    return SceneFlag == MAIN_SCENE && Hero && SocketClient && SocketClient->IsConnected();
}

bool ReadInteger(const Json& object, const char* key, int minimum, int maximum, int& result)
{
    const auto value = object.find(key);
    if (value == object.end() || !value->is_number_integer() || *value < minimum || *value > maximum)
        return false;
    result = value->get<int>();
    return true;
}

bool ReadText(const Json& object, const char* key, std::wstring& result)
{
    const auto value = object.find(key);
    if (value == object.end() || !value->is_string() || value->get_ref<const std::string&>().size() > MaximumTextBytes)
        return false;
    result = StringUtils::NarrowToWide(value->get_ref<const std::string&>());
    return true;
}

// An offer for a rank this client has no title for (the server got new titles first) is left out.
bool ReadOffer(const Json& value, std::vector<Offer>& list)
{
    Offer offer;
    if (!value.is_object() || !ReadInteger(value, "Rank", 1, MaximumRank, offer.rank)
        || !ReadInteger(value, "Price", 0, INT_MAX, offer.price))
        return false;
    const auto owned = value.find("Owned");
    if (owned == value.end() || !owned->is_boolean())
        return false;
    offer.owned = owned->get<bool>();
    if (value.contains("Name") && !ReadText(value, "Name", offer.name))
        return false;
    if (std::any_of(list.begin(), list.end(), [&offer](const Offer& listed) { return listed.rank == offer.rank; }))
        return false;
    if (Data::RankTitles::Find(offer.rank))
        list.push_back(std::move(offer));
    return true;
}

bool ReadOffers(const Json& json, std::vector<Offer>& list)
{
    const auto ranks = json.find("Ranks");
    if (ranks == json.end() || !ranks->is_array() || ranks->size() > MaximumOffers)
        return false;
    for (const Json& value : *ranks)
    {
        if (!ReadOffer(value, list))
            return false;
    }
    return true;
}

void WriteInt32(BYTE* target, std::uint32_t value)
{
    for (int byte = 0; byte < 4; ++byte)
        target[byte] = static_cast<BYTE>((value >> (8 * byte)) & 0xFF);
}

bool Send(Operation operation, int rank, int price)
{
    if (pending || !Ready())
        return false;
    pending = ++sequence;
    if (!pending)
        pending = ++sequence;
    owner = Hero->ID;
    sentAt = SDL_GetTicks();
    BYTE packet[RequestSize] = {0xC1, RequestSize, PacketHeadCode, PacketSubCode, static_cast<BYTE>(operation),
                                static_cast<BYTE>(rank & 0xFF), static_cast<BYTE>((rank >> 8) & 0xFF)};
    WriteInt32(packet + PriceOffset, static_cast<std::uint32_t>(price));
    WriteInt32(packet + RequestIdOffset, pending);
    SocketClient->Send(packet, sizeof(packet));
    return true;
}
} // namespace

bool Load()
{
    return Send(Operation::Load, 0, 0);
}

bool Buy(const Offer& offer)
{
    if (!balance || offer.owned || offer.price <= 0 || offer.price > *balance)
        return false;
    return Send(Operation::Buy, offer.rank, offer.price);
}

bool Equip(int rank)
{
    const bool owned = std::any_of(offers.begin(), offers.end(),
                                   [rank](const Offer& offer) { return offer.rank == rank && offer.owned; });
    if (rank != 0 && !owned)
        return false;
    return Send(Operation::Equip, rank, 0);
}

void Receive(std::span<const std::uint8_t> packet)
{
    if (!pending || !Ready() || owner != Hero->ID || packet.size() <= HeaderSize || packet[0] != 0xC2
        || packet[3] != PacketHeadCode || packet[4] != PacketSubCode
        || ((packet[1] << 8) | packet[2]) != static_cast<int>(packet.size()))
        return;
    const Json json = Json::parse(packet.begin() + HeaderSize, packet.end(), nullptr, false);
    if (!json.is_object() || !json.contains("RequestId") || !json["RequestId"].is_number_unsigned()
        || json["RequestId"] != pending)
        return;
    int confirmedBalance = 0;
    int confirmedActive = 0;
    std::wstring confirmedStatus;
    std::vector<Offer> confirmedOffers;
    if (!ReadInteger(json, "Balance", 0, INT_MAX, confirmedBalance)
        || !ReadInteger(json, "Active", 0, MaximumRank, confirmedActive) || !ReadText(json, "Status", confirmedStatus)
        || !ReadOffers(json, confirmedOffers))
        return;
    offers = std::move(confirmedOffers);
    balance = confirmedBalance;
    active = confirmedActive;
    ::GameLogic::RankTitles::SetOwnRank(active);
    status = std::move(confirmedStatus);
    pending = 0;
    ++revision;
}

void Update()
{
    if (!Ready() || (!owner.empty() && owner != Hero->ID))
    {
        Reset();
        return;
    }
    if (pending && SDL_GetTicks() - sentAt >= TimeoutMs)
    {
        pending = 0;
        balance.reset();
        status = L"The server did not answer. Refresh the shop before buying again.";
        ++revision;
    }
}

void Reset()
{
    if (owner.empty() && offers.empty() && !pending)
        return;
    offers.clear();
    status.clear();
    owner.clear();
    pending = 0;
    balance.reset();
    active = 0;
    ++revision;
}

bool Pending()
{
    return pending != 0;
}

unsigned Revision()
{
    return revision;
}

std::optional<int> Balance()
{
    return balance;
}

int Active()
{
    return active;
}

const std::wstring& Status()
{
    return status;
}

std::span<const Offer> Offers()
{
    return offers;
}
} // namespace GameLogic::RankTitles::Shop

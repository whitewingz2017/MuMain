#include "stdafx.h"
#include "GameLogic/Items/StoreMail.h"
#include "GameLogic/Items/PersonalShopTitleImp.h"
#include "Dotnet/Connection.h"
#include "Network/Server/WSclient.h"
#include "Core/Utilities/StringUtils.h"
#include <array>
#include <set>

namespace GameLogic::PersonalShop
{
namespace
{
std::set<unsigned int> Attachments;
uint32_t Read32(const uint8_t* p) { return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24; }
void Write32(uint8_t* p, uint32_t n) { for (int i=0; i<4; ++i) p[i] = static_cast<uint8_t>(n >> (8*i)); }
}
void SendCombinedPrice(int slot, const ItemPrice& price)
{
    if (!SocketClient || !SocketClient->IsConnected() || price.Components.empty() || price.Components.size() > MaximumPriceParts) return;
    std::array<BYTE, 54> packet{0xC1, 54, 0xFE, 1, static_cast<BYTE>(slot), static_cast<BYTE>(price.Components.size())};
    for (size_t i=0; i<price.Components.size(); ++i)
    {
        const auto& part = price.Components[i];
        packet[6+i*6] = part.type & 255; packet[7+i*6] = part.type >> 8;
        Write32(packet.data()+8+i*6, part.amount);
    }
    SocketClient->Send(packet.data(), static_cast<int>(packet.size()));
}
void ReceiveStoreMail(std::span<const uint8_t> packet)
{
    if (packet.size() == 7 && packet[3] == 2)
    {
        const unsigned int index = packet[4] | (packet[5] << 8);
        if (packet[6]) Attachments.insert(index); else Attachments.erase(index);
    }
    if (packet.size() != 55 || packet[3] != 1 || packet[6] == 0 || packet[6] > MaximumPriceParts) return;
    ItemPrice price{1, PriceKind::Item, CombinedPriceMarker};
    for (size_t i=0; i<packet[6]; ++i)
    {
        const auto* part = packet.data()+7+i*6;
        const auto amount = Read32(part+2);
        if (amount == 0 || amount > INT_MAX) return;
        price.Components.push_back({static_cast<uint16_t>(part[0] | part[1] << 8), static_cast<int>(amount)});
    }
    AddPersonalItemPrice(packet[5], price, packet[4] ? PSHOPWNDTYPE_SALE : PSHOPWNDTYPE_PURCHASE);
}
bool HasMailAttachment(unsigned int index) { return Attachments.contains(index); }
void SendMailPurchase(unsigned int playerId, std::wstring_view name, int slot)
{
    if (!SocketClient || !SocketClient->IsConnected()) return;
    std::array<BYTE,17> packet{0xC1,17,0xFE,3,static_cast<BYTE>(playerId),static_cast<BYTE>(playerId >> 8)};
    const auto encoded = StringUtils::WideToNarrow(std::wstring(name).c_str());
    for (size_t i=0; i<encoded.size() && i<10; ++i) packet[6+i] = static_cast<BYTE>(encoded[i]);
    packet[16] = static_cast<BYTE>(slot);
    SocketClient->Send(packet.data(), static_cast<int>(packet.size()));
}
void ClaimMailAttachment(unsigned int index)
{
    if (!SocketClient || !SocketClient->IsConnected() || !HasMailAttachment(index) || index > 65535) return;
    const std::array<BYTE,6> packet{0xC1,6,0xFE,2,static_cast<BYTE>(index),static_cast<BYTE>(index >> 8)};
    SocketClient->Send(packet.data(), static_cast<int>(packet.size()));
}
}

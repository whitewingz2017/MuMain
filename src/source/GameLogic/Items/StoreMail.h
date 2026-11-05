#pragma once
#include "GameLogic/Items/PersonalShopCurrency.h"
#include <span>
namespace GameLogic::PersonalShop
{
void SendCombinedPrice(int slot, const ItemPrice& price);
void SendMailPurchase(unsigned int playerId, std::wstring_view name, int slot);
void ReceiveStoreMail(std::span<const uint8_t> packet);
bool HasMailAttachment(unsigned int letterIndex);
void ClaimMailAttachment(unsigned int letterIndex);
}

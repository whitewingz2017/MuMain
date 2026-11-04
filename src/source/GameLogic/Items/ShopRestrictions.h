#pragma once

typedef struct tagITEM ITEM;

namespace GameLogic::Items
{
    bool IsPersonalShopBan(const ITEM* pItem);
    bool IsSellingBan(const ITEM* pItem);
    bool IsRepairBan(const ITEM* pItem);
    bool CanConfirmSale(const ITEM* item, int sourceIndex, int expectedIndex, unsigned int expectedKey, bool shopOpen, bool salePending);
}

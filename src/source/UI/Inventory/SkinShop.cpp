#include "stdafx.h"
#include "UI/Inventory/SkinShop.h"
#include "UI/Inventory/SkinShopWindow.h"
#include "GameLogic/Cosmetics/SkinShop.h"

namespace UI::Inventory::SkinShop
{
void Open()
{
    if (mu::ui::window::g_pSkinShopWindow) mu::ui::window::g_pSkinShopWindow->Open();
}
void Reset()
{
    if (mu::ui::window::g_pSkinShopWindow) mu::ui::window::g_pSkinShopWindow->Show(false);
    GameLogic::Cosmetics::SkinShop::Reset();
}
}

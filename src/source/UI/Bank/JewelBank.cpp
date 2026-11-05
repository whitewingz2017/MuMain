#include "stdafx.h"
#include "UI/Bank/JewelBank.h"
#include "UI/Dialogs/GenericMenuDialog.h"
#include "Core/Utilities/StringUtils.h"
#include "UI/Inventory/ItemCameraTarget.h"
#include "Engine/Object/ZzzInventory.h"
#include <memory>
#include <array>
#include <algorithm>
namespace UI::Bank
{
namespace
{
unsigned transferQuantity = 10;
void Request(std::uint8_t operation, std::uint16_t item = 0, std::uint32_t amount = 0)
{
    if (GameLogic::Bank::Request(operation, item, amount)) return;
    mu::ui::window::GenericMenuConfig config;
    config.title = L"Jewel Bank";
    config.kind = "jewel-bank";
    config.lines.push_back({L"Jewel Bank requires an active game connection. Reconnect, then try again."});
    mu::ui::window::GenericMenuConfig::MenuButton close;
    close.label = L"Close"; close.compact = true; close.dismiss = true;
    config.buttons.push_back(std::move(close));
    if (mu::ui::window::g_pGenericMenuDialog) mu::ui::window::g_pGenericMenuDialog->ShowOrRefresh(std::move(config));
}
void Actions(const GameLogic::Bank::JewelRow& row)
{
    using namespace mu::ui::window;
    GenericMenuConfig config;
    config.title = StringUtils::NarrowToWide(row.name);
    config.kind = "jewel-bank";
    config.keepOpen = true;
    config.lines.push_back({L"Bank: " + std::to_wstring(row.balance) + L" | Inventory: " + std::to_wstring(row.inventory)});
    constexpr std::array<const wchar_t*, 6> labels = {L"Deposit all", L"Withdraw 1", L"Withdraw 10", L"Withdraw 20", L"Withdraw 30", L"Withdraw all"};
    constexpr std::array<unsigned, 6> amounts = {0, 1, 10, 20, 30, 0};
    for (int i = 0; i < 6; ++i)
    {
        GenericMenuConfig::MenuButton button;
        button.label = labels[i]; button.compact = true;
        button.onClick = [item = row.item, operation = i == 0 ? 1 : 2, amount = amounts[i]] { Request(operation, item, amount); };
        config.buttons.push_back(std::move(button));
    }
    GenericMenuConfig::MenuButton back;
    back.label = L"Back"; back.compact = true; back.onClick = Open;
    config.buttons.push_back(std::move(back));
    g_pGenericMenuDialog->ShowOrRefresh(std::move(config));
}
mu::ui::window::GenericMenuConfig::MenuButton JewelButton(const GameLogic::Bank::JewelRow& row)
{
    mu::ui::window::GenericMenuConfig::MenuButton button;
    button.label = StringUtils::NarrowToWide(row.name);
    button.detail = std::to_wstring(row.balance);
    button.tooltip = L"Inventory: " + std::to_wstring(row.inventory) + L". + deposits 1; - withdraws 1. Click the jewel for more amounts.";
    auto icon = std::make_shared<UI::Items::ItemCameraTarget>(
        [type = row.item](const Rml::Vector2f& offset, const Rml::Vector2f& size)
        { RenderItem3DWithHover(offset.x, offset.y, size.x, size.y, type, 0, 0, 0, false); });
    icon->SetContinuous(false);
    button.syncIcon = [icon](Rml::Element* image) { icon->Sync(image, image != nullptr); };
    button.onClick = [row] { Actions(row); };
    button.secondaryLabel = L"Deposit"; button.tertiaryLabel = L"Withdraw";
    button.onSecondary = [item = row.item, amount = transferQuantity] { Request(1, item, amount); };
    button.onTertiary = [item = row.item, amount = transferQuantity] { Request(2, item, amount); };
    button.onIncrement = [item = row.item] { Request(1, item, 1); };
    button.onDecrement = [item = row.item] { Request(2, item, 1); };
    return button;
}
}
void Open() { Request(0); }
void Show(const GameLogic::Bank::Snapshot& snapshot)
{
    using namespace mu::ui::window;
    if (!g_pGenericMenuDialog) return;
    GenericMenuConfig config;
    config.title = L"Jewels Bank";
    config.kind = "jewel-bank";
    config.keepOpen = true;
    config.lines.push_back({StringUtils::NarrowToWide(snapshot.status)});
    for (const int direction : {-1, 0, 1})
    {
        GenericMenuConfig::MenuButton quantity;
        quantity.compact = true;
        quantity.label = direction < 0 ? L"-10" : direction > 0 ? L"+10" : L"Pack: " + std::to_wstring(transferQuantity);
        quantity.onClick = [snapshot,direction]
        {
            transferQuantity = std::clamp(static_cast<int>(transferQuantity) + direction * 10, 10, 50);
            Show(snapshot);
        };
        config.buttons.push_back(std::move(quantity));
    }
    for (const auto& row : snapshot.rows)
        config.buttons.push_back(JewelButton(row));
    GenericMenuConfig::MenuButton refresh;
    refresh.label = L"Refresh"; refresh.compact = true; refresh.onClick = Open;
    config.buttons.push_back(std::move(refresh));
    GenericMenuConfig::MenuButton close;
    close.label = L"Close"; close.compact = true; close.dismiss = true;
    config.buttons.push_back(std::move(close));
    g_pGenericMenuDialog->ShowOrRefresh(std::move(config));
}
}

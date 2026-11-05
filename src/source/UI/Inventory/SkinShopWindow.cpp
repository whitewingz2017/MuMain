#include "stdafx.h"
#include "UI/Inventory/SkinShopWindow.h"
#include "UI/Inventory/SkinInventoryWindow.h"
#include "GameLogic/Cosmetics/SkinShop.h"
#include "GameLogic/Cosmetics/CosmeticArmor.h"
#include "UI/Core/WindowManager.h"
#include "UI/Inventory/InventoryCtrl.h"
#include "Core/Utilities/StringUtils.h"
#include "Core/Input/KeyState.h"
#include "Engine/Object/ZzzInventory.h"
#include "Render/Items/ScopedItemEffects.h"
#include <RmlUi/Core.h>

namespace mu::ui::window
{
namespace
{
namespace Shop = GameLogic::Cosmetics::SkinShop;
constexpr int CategoryCount = 6;
constexpr float TurnStep = 45.f;
int Category(const Shop::Item& item)
{
    if (GameLogic::Cosmetics::IsFenrirMount(item.type)) return 5;
    return item.part == 1 ? 0 : item.part == 7 ? 1 : item.type / 512 == 6 ? 3 : item.part == 8 ? 4 : 2;
}
}

CSkinShopWindow* g_pSkinShopWindow = nullptr;
CSkinShopWindow::~CSkinShopWindow() { Release(); }

void CSkinShopWindow::Create(CManager* manager)
{
    m_manager = manager;
    m_visible.fill(-1);
    for (int slot = 0; slot < PageSize; ++slot)
        m_itemTargets[slot] = std::make_unique<UI::Items::ItemCameraTarget>(
            [this, slot](const Rml::Vector2f& offset, const Rml::Vector2f& size) { DrawItem(slot, offset, size); });
    for (auto& target : m_itemTargets) target->SetContinuous(false);
    m_view.Ensure();
    manager->AddUIObj(INTERFACE_SKIN_SHOP, this);
    Show(false);
}

void CSkinShopWindow::Release()
{
    Show(false);
    m_view.Release();
    for (auto& target : m_itemTargets) target.reset();
    if (m_manager) m_manager->RemoveUIObj(this);
    m_manager = nullptr;
}

void CSkinShopWindow::Bind(Rml::DataModelConstructor& constructor, Model& model)
{
    auto entry = constructor.RegisterStruct<Entry>();
    entry.RegisterMember("index", &Entry::index);
    entry.RegisterMember("label", &Entry::label);
    entry.RegisterMember("price", &Entry::price);
    entry.RegisterMember("selected", &Entry::selected);
    constructor.RegisterArray<std::vector<Entry>>();
    constructor.Bind("items", &model.items);
    constructor.Bind("weapon_types", &model.weaponTypes);
    constructor.Bind("weapon_label", &model.weaponLabel);
    constructor.Bind("bonus_label", &model.bonusLabel);
    constructor.Bind("open_dropdown", &model.openDropdown);
    model.weaponTypes.clear();
    const char* weaponLabels[] = {"All weapons", "Swords", "Maces", "Spears", "Bows", "Staffs / sticks", "Scepters", "Books", "RF gloves", "Axes"};
    for (int family = 0; family < std::size(weaponLabels); ++family)
        model.weaponTypes.push_back({family, weaponLabels[family], {}, false});
    constructor.Bind("bonuses", &model.bonuses);
    constructor.Bind("title", &model.title);
    constructor.Bind("has_title", &model.hasTitle);
    constructor.Bind("positioned", &model.positioned);
    constructor.Bind("root_x", &model.rootX);
    constructor.Bind("root_y", &model.rootY);
    constructor.Bind("balance", &model.balance);
    constructor.Bind("status", &model.status);
    constructor.Bind("selected_name", &model.selectedName);
    constructor.Bind("price", &model.price);
    constructor.Bind("note", &model.note);
    constructor.Bind("page", &model.page);
    constructor.Bind("category", &model.category);
    constructor.Bind("preview_level", &model.previewLevel);
    constructor.Bind("pending", &model.pending);
    constructor.Bind("can_buy", &model.canBuy);
    constructor.Bind("previous", &model.previous);
    constructor.Bind("next", &model.next);
    BindActions(constructor);
}

void CSkinShopWindow::BindActions(Rml::DataModelConstructor& constructor)
{
    constructor.BindEventCallback("skin_dropdown", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args) {
        const int dropdown = args.size() == 1 ? args[0].Get<int>(0) : 0;
        if (Shop::Pending() || dropdown < 1 || dropdown > 2) return;
        auto& open = m_view.GetModel().openDropdown;
        open = open == dropdown ? 0 : dropdown;
        m_view.MarkDirty("open_dropdown");
    });
    constructor.BindEventCallback("skin_weapon_filter", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args) {
        const int family = args.size() == 1 ? args[0].Get<int>(-1) : -1;
        if (Shop::Pending() || family < 0 || family >= m_view.GetModel().weaponTypes.size()) return;
        m_weaponFilter = family;
        m_view.GetModel().weaponLabel = m_view.GetModel().weaponTypes[family].label;
        m_view.MarkDirty("weapon_label");
        m_page = 0;
        RebuildItems();
        Select(-1);
    });
    constructor.BindEventCallback("skin_close", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { Show(false); });
    constructor.BindEventCallback("skin_refresh", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { Load(); });
    constructor.BindEventCallback("skin_buy", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) { Buy(); });
    constructor.BindEventCallback("skin_category", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args) {
        const int category = args.size() == 1 ? args[0].Get<int>(-1) : -1;
        if (category < 0 || category >= CategoryCount || Shop::Pending()) return;
        m_view.GetModel().category = category;
        m_view.MarkDirty("category");
        m_page = 0;
        RebuildItems();
        Select(-1);
    });
    constructor.BindEventCallback("skin_page", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args) {
        const int delta = args.size() == 1 ? args[0].Get<int>(0) : 0;
        const auto& model = m_view.GetModel();
        if (Shop::Pending() || (delta == -1 ? !model.previous : delta != 1 || !model.next)) return;
        m_page += delta;
        RebuildItems();
    });
    constructor.BindEventCallback("skin_select", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args) {
        if (args.size() == 1 && !Shop::Pending()) Select(args[0].Get<int>(-1));
    });
    constructor.BindEventCallback("skin_bonus", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args) {
        if (args.size() == 1 && !Shop::Pending()) SelectBonus(args[0].Get<int>(-1));
    });
    constructor.BindEventCallback("skin_level", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args) {
        if (args.size() == 1) SetPreviewLevel(args[0].Get<int>(0));
    });
    constructor.BindEventCallback("skin_turn", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& args) {
        const int direction = args.size() == 1 ? args[0].Get<int>(0) : 0;
        if (m_preview && (direction == -1 || direction == 1)) m_preview->Turn(direction * TurnStep);
    });
}

void CSkinShopWindow::SetPreviewLevel(int delta)
{
    if (!m_preview || m_selected < 0 || (delta != -1 && delta != 1)) return;
    auto& level = m_view.GetModel().previewLevel;
    level = std::clamp(level + delta, 0, 15);
    m_preview->SetLevel(level);
    m_view.MarkDirty("preview_level");
}

void CSkinShopWindow::Open()
{
    if (!GameLogic::Cosmetics::Ready()) return;
    if (g_pSkinInventoryWindow) g_pSkinInventoryWindow->Show(false);
    CInventoryCtrl::BackupPickedItem();
    m_view.Ensure();
    if (!m_preview) m_preview = std::make_unique<UI::Inventory::SkinShopPreview>();
    Show(true);
    Load();
}

void CSkinShopWindow::Show(bool visible)
{
    CObject::Show(visible);
    if (visible && m_view.Document()) m_view.Document()->Show(Rml::ModalFlag::Modal, Rml::FocusFlag::Document);
    else m_view.Hide();
    if (!visible)
    {
        for (auto& target : m_itemTargets) if (target) target->Disable();
        m_preview.reset();
        // Reopened, the shop starts with nothing previewed, as its new preview does.
        m_selected = m_selectedType = -1;
    }
}

void CSkinShopWindow::Load()
{
    if (Shop::Pending()) return;
    m_revision = Shop::Revision();
    if (Shop::Load())
    {
        m_view.GetModel().status = "Loading skins...";
        m_view.MarkDirty("status");
    }
}

void CSkinShopWindow::RebuildItems()
{
    auto& model = m_view.GetModel();
    model.items.clear();
    m_visible.fill(-1);
    for (auto& target : m_itemTargets) target->Invalidate();
    const auto& items = Shop::Items();
    // Skins the character's class can't wear aren't listed.
    const auto listed = [&](const Shop::Item& item)
    {
        return item.eligible && Category(item) == model.category
            && (model.category != 2 || Shop::MatchesWeaponFilter(item, m_weaponFilter));
    };
    // The previewed skin stays selected across pages and reloads; a reload may move its index.
    m_selected = -1;
    for (int i = 0; i < static_cast<int>(items.size()); ++i)
        if (items[i].type == m_selectedType) m_selected = i;
    const int listedCount = static_cast<int>(std::count_if(items.begin(), items.end(), listed));
    m_page = std::clamp(m_page, 0, std::max(0, (listedCount - 1) / PageSize));
    int count = 0;
    int index = 0;
    for (const auto& item : items)
    {
        if (listed(item))
        {
            if (count >= m_page * PageSize && count < (m_page + 1) * PageSize)
            {
                m_visible[model.items.size()] = index;
                model.items.push_back({index, StringUtils::WideToNarrow(item.name.c_str()), std::to_string(item.price) + " WCoinC", index == m_selected});
            }
            ++count;
        }
        ++index;
    }
    model.previous = m_page > 0;
    model.next = count > (m_page + 1) * PageSize;
    model.page = std::to_string(m_page + 1) + " / " + std::to_string(std::max(1, (count + PageSize - 1) / PageSize));
    for (const char* field : {"items", "previous", "next", "page"}) m_view.MarkDirty(field);
    // A reloaded catalog without the previewed skin clears the preview.
    if (m_selected < 0 && m_selectedType >= 0) Select(-1);
}

void CSkinShopWindow::Select(int index)
{
    if (index >= 0 && std::find(m_visible.begin(), m_visible.end(), index) == m_visible.end()) return;
    m_selected = index;
    m_selectedType = index < 0 ? -1 : Shop::Items()[index].type;
    m_bonus = 0;
    m_view.GetModel().previewLevel = 0;
    m_view.MarkDirty("preview_level");
    m_view.GetModel().openDropdown = 0;
    m_view.MarkDirty("open_dropdown");
    if (m_preview) m_preview->Select(index < 0 ? -1 : Shop::Items()[index].type);
    for (auto& item : m_view.GetModel().items) item.selected = item.index == index;
    m_view.MarkDirty("items");
    UpdateDetails();
}

void CSkinShopWindow::SelectBonus(int number)
{
    if (m_selected < 0 || m_selected >= static_cast<int>(Shop::Items().size())) return;
    const auto choices = Shop::Bonuses(Shop::Items()[m_selected]);
    if (std::none_of(choices.begin(), choices.end(), [number](const Shop::Bonus& option) { return option.number == number; })) return;
    m_bonus = number;
    m_view.GetModel().openDropdown = 0;
    m_view.MarkDirty("open_dropdown");
    UpdateDetails();
}

void CSkinShopWindow::UpdateDetails()
{
    auto& model = m_view.GetModel();
    model.bonuses.clear();
    model.bonusLabel = "Choose bonus";
    model.canBuy = false;
    model.price.clear();
    model.note = "Select a skin to preview.";
    model.selectedName = "Character preview";
    if (m_selected >= 0 && m_selected < static_cast<int>(Shop::Items().size()))
    {
        const auto& item = Shop::Items()[m_selected];
        const auto choices = Shop::Bonuses(item);
        model.selectedName = StringUtils::WideToNarrow(item.name.c_str());
        model.price = std::to_string(item.price) + " WCoinC";
        model.note = !item.eligible ? "Unavailable for your class." : item.price <= 0 ? "Preview only: not for sale."
            : GameLogic::Cosmetics::IsFenrirMount(item.type) ? "Rideable Fenrir mount. Equip in Gear helper slot."
            : choices.empty() ? "Permanent cosmetic pet. No combat bonus." : "Choose one bonus. Permanent and tradable.";
        if (choices.empty()) model.bonusLabel = "No bonus";
        for (const auto& bonus : choices)
        {
            const auto label = StringUtils::WideToNarrow(bonus.label.c_str());
            model.bonuses.push_back({bonus.number, label, {}, bonus.number == m_bonus});
            if (bonus.number == m_bonus) model.bonusLabel = label;
        }
        model.canBuy = !Shop::Pending() && item.eligible && item.price > 0 && item.price <= Shop::Balance() && (choices.empty() || m_bonus > 0);
    }
    for (const char* field : {"bonuses", "bonus_label", "can_buy", "price", "note", "selected_name"}) m_view.MarkDirty(field);
}

void CSkinShopWindow::Buy()
{
    if (!m_view.GetModel().canBuy || m_selected < 0 || m_selected >= static_cast<int>(Shop::Items().size())) return;
    if (Shop::Purchase(Shop::Items()[m_selected], m_bonus, m_view.GetModel().previewLevel))
    {
        m_view.GetModel().status = "Purchasing...";
        m_view.MarkDirty("status");
        UpdateDetails();
    }
}

bool CSkinShopWindow::Update()
{
    Shop::Update();
    if (!IsVisible()) return true;
    if (!GameLogic::Cosmetics::Ready())
    {
        Show(false);
        return true;
    }
    if (m_revision != Shop::Revision())
    {
        m_revision = Shop::Revision();
        auto& model = m_view.GetModel();
        model.balance = "WCoinC: " + std::to_string(Shop::Balance());
        model.status = Shop::Status() == L"Ready" ? "Select an item, preview, then choose its bonus." : StringUtils::WideToNarrow(Shop::Status().c_str());
        m_view.MarkDirty("balance");
        m_view.MarkDirty("status");
        // A purchase (or a refresh) keeps the category, page and previewed skin.
        RebuildItems();
        UpdateDetails();
    }
    auto& model = m_view.GetModel();
    if (model.pending != Shop::Pending())
    {
        model.pending = Shop::Pending();
        m_view.MarkDirty("pending");
        UpdateDetails();
    }
    // The preview's character is posed, lit and turned in the update; the draw only draws it.
    if (m_preview) m_preview->Update();
    return true;
}

void CSkinShopWindow::DrawItem(int slot, const Rml::Vector2f& offset, const Rml::Vector2f& size)
{
    const int index = m_visible[slot];
    if (index < 0 || index >= static_cast<int>(Shop::Items().size())) return;
    const Render::Items::ScopedItemEffects effects(false);
    RenderItem3DWithHover(offset.x, offset.y, size.x, size.y, Shop::Items()[index].type, 0, 0, 0, false);
}

void CSkinShopWindow::SyncTargets()
{
    auto* document = m_view.Document();
    if (!document) return;
    for (int slot = 0; slot < PageSize; ++slot)
        m_itemTargets[slot]->Sync(document->GetElementById("skin_item_" + std::to_string(slot)), IsVisible() && m_visible[slot] >= 0);
    if (m_preview) m_preview->Sync(document->GetElementById("skin_character"));
}

bool CSkinShopWindow::Render()
{
    SyncTargets();
    return true;
}

bool CSkinShopWindow::UpdateKeyEvent()
{
    if (!IsVisible()) return true;
    if (IsPress(VK_ESCAPE))
    {
        if (m_view.GetModel().openDropdown != 0)
        {
            m_view.GetModel().openDropdown = 0;
            m_view.MarkDirty("open_dropdown");
        }
        else Show(false);
    }
    return false;
}
}

#include "stdafx.h"
#include "UI/Inventory/MuunInventoryWindow.h"
#include "UI/Inventory/InventoryCtrl.h"
#include "UI/Inventory/InventoryContents.h"
#include "UI/Inventory/InventoryExtension.h"
#include "UI/Core/WindowManager.h"
#include "UI/Core/WindowSystem.h"
#include "UI/RmlBridge/RmlPointer.h"
#include "UI/RmlBridge/RmlElementBox.h"
#include "UI/Dialogs/GenericMenuDialog.h"
#include "UI/Dialogs/GenericConfirmDialog.h"
#include "GameLogic/Pets/Muun.h"
#include "GameLogic/Pets/MuunDrop.h"
#include "Core/Utilities/StringUtils.h"
#include "Core/Input/KeyState.h"
#include "Engine/Object/ZzzInventory.h"
#include "Render/Items/ScopedItemEffects.h"
#include <RmlUi/Core.h>
namespace mu::ui::window {
namespace Muun = GameLogic::Pets::Muun;
CMuunInventoryWindow* g_pMuunInventoryWindow = nullptr;
namespace {
Rml::Vector2f EventPoint(const Rml::Event& event) {
    return {event.GetParameter<float>("mouse_x", -1), event.GetParameter<float>("mouse_y", -1)};
}
}
CMuunInventoryWindow::~CMuunInventoryWindow() { Release(); }
void CMuunInventoryWindow::Create(CManager* manager) {
    m_manager = manager; m_visible.fill(-1);
    for (int i = 0; i < PageSize; ++i) {
        m_targets[i] = std::make_unique<UI::Items::ItemCameraTarget>([this, i](const auto& p, const auto& s) { DrawItem(i, p, s); });
        m_targets[i]->SetContinuous(false);
    }
    manager->AddUIObj(INTERFACE_MUUN_INVENTORY, this); Show(false);
}
void CMuunInventoryWindow::Release() {
    Show(false); m_view.Release();
    for (auto& target : m_targets) target.reset();
    if (m_manager) m_manager->RemoveUIObj(this);
    m_manager = nullptr;
}
void CMuunInventoryWindow::Bind(Rml::DataModelConstructor& c, Model& m) {
    auto entry = c.RegisterStruct<Entry>();
    entry.RegisterMember("position", &Entry::position); entry.RegisterMember("index", &Entry::index); entry.RegisterMember("label", &Entry::label);
    c.RegisterArray<std::vector<Entry>>(); c.Bind("items", &m.items);
    c.Bind("title", &m.title);
    c.Bind("page", &m.page); c.Bind("status", &m.status); c.Bind("pending", &m.pending);
    c.BindEventCallback("muun_close", [this](auto, auto&, const auto&) { Show(false); });
    c.BindEventCallback("muun_page", [this](auto, auto&, const Rml::VariantList& a) {
        if (a.size() == 1 && (a[0].Get<int>() == -1 || a[0].Get<int>() == 1)) { m_page += a[0].Get<int>(); Rebuild(); }
    });
    c.BindEventCallback("muun_drag_start", [this](auto, auto&, const Rml::VariantList& a) {
        if (a.size() == 1) BeginDrag(a[0].Get<int>(-1));
    });
    c.BindEventCallback("muun_drag_end", [this](auto, auto& event, const auto&) { EndDrag(EventPoint(event)); });
    c.BindEventCallback("muun_drop", [this](auto, auto& event, const auto&) {
        if (event.template GetParameter<int>("button", -1) == 0) DropBagItem(EventPoint(event));
    });
    c.BindEventCallback("muun_book", [this](auto, auto&, const auto&) { ShowBook(); });
    c.BindEventCallback("muun_exchange", [this](auto, auto&, const auto&) { ShowExchanges(); });
    c.BindEventCallback("muun_refresh", [](auto, auto&, const auto&) { Muun::Request(); });
}
void CMuunInventoryWindow::Open() {
    if (SceneFlag != MAIN_SCENE || !Hero) return;
    g_pNewUISystem->Show(INTERFACE_INVENTORY); m_view.Ensure(); m_page = 0;
    Show(true); Rebuild(); Muun::Request();
}
void CMuunInventoryWindow::Show(bool visible) {
    CObject::Show(visible);
    if (visible && m_view.Document()) m_view.Document()->Show(); else m_view.Hide();
    if (!visible) {
        m_dragSlot = -1;
        for (auto& target : m_targets) if (target) target->Disable();
    }
}
void CMuunInventoryWindow::Rebuild() {
    auto& m = m_view.GetModel(); const auto& state = Muun::Get();
    constexpr int StoredPageSize = 20;
    const int pages = std::max(1, (state.capacity - 2 + StoredPageSize - 1) / StoredPageSize);
    m_page = std::clamp(m_page, 0, pages - 1); m.items.clear(); m_visible.fill(-1);
    for (int position = 0; position < PageSize; ++position) {
        const int slot = position < 2 ? position : 2 + m_page * StoredPageSize + position - 2;
        const auto item = std::find_if(state.items.begin(), state.items.end(), [slot](const auto& i) { return i.slot == slot; });
        if (item != state.items.end()) {
            m_visible[position] = static_cast<int>(item - state.items.begin());
            m.items.push_back({slot, position, item->name});
        } else m.items.push_back({-1, position, ""});
    }
    for (auto& target : m_targets) if (target) target->Invalidate();
    m.page = std::to_string(m_page + 1) + " / " + std::to_string(pages);
    m.status = state.status.empty() ? "Drag to Main/Sub or storage. Drop a duplicate, life jewel or evolution stone onto a Muun." : state.status;
    for (const char* field : {"items", "page", "status"}) m_view.MarkDirty(field);
}
void CMuunInventoryWindow::Confirm(int operation, int source, int target, const std::wstring& text) {
    if (!g_pGenericConfirmDialog) return;
    GenericDialogConfig cfg; cfg.primaryLabel = L"Confirm"; cfg.showCancel = true; cfg.cancelLabel = L"Cancel"; cfg.lines.push_back({text});
    cfg.onPrimary = [operation,source,target] { Muun::Request(operation, source, target); };
    cfg.isValid = [this] { return IsVisible() && SceneFlag == MAIN_SCENE && !Muun::Get().pending; };
    g_pGenericConfirmDialog->Show(std::move(cfg));
}
void CMuunInventoryWindow::ShowBook(int page) {
    if (!IsVisible() || !g_pGenericMenuDialog) return;
    const auto& rules = Muun::Get().species;
    constexpr int PageCount = 6;
    const int pages = std::max(1, (static_cast<int>(rules.size()) + PageCount - 1) / PageCount);
    page = std::clamp(page, 0, pages - 1);
    GenericMenuConfig menu; menu.title = L"Pet Book " + std::to_wstring(page + 1) + L" / " + std::to_wstring(pages);
    for (int i = page * PageCount; i < std::min<int>((page + 1) * PageCount, rules.size()); ++i) {
        const auto& rule = rules[i]; GenericMenuConfig::MenuButton entry;
        entry.label = StringUtils::NarrowToWide(rule.name.empty() ? std::to_string(rule.type) : rule.name) + (rule.owned ? L" [Owned]" : L"");
        entry.detail = StringUtils::NarrowToWide(rule.description); menu.buttons.push_back(std::move(entry));
    }
    for (const int step : {-1, 1}) if (page + step >= 0 && page + step < pages) {
        GenericMenuConfig::MenuButton b; b.label = step < 0 ? L"Previous" : L"Next";
        b.onClick = [this,page,step] { ShowBook(page + step); }; menu.buttons.push_back(std::move(b));
    }
    GenericMenuConfig::MenuButton close; close.label = L"Close"; close.dismiss = true; menu.buttons.push_back(std::move(close));
    g_pGenericMenuDialog->Show(std::move(menu));
}
void CMuunInventoryWindow::ShowExchanges() {
    if (!IsVisible() || !g_pGenericMenuDialog) return;
    GenericMenuConfig menu; menu.title = L"Muun egg / material exchange";
    for (const auto& recipe : Muun::Get().exchanges) {
        int source = -1;
        if (g_pMyInventory) for (int i = MAX_EQUIPMENT_INDEX; i < MAX_MY_INVENTORY_EX_INDEX; ++i) {
            const auto* item = UI::Inventory::FindPlayerItem(i);
            if (item && item->Type == recipe.material) { source = i; break; }
        }
        GenericMenuConfig::MenuButton b; b.label = StringUtils::NarrowToWide(recipe.name);
        b.detail = L"Consumes " + std::to_wstring(recipe.count) + L" material(s). Reward goes to Muun Inventory.";
        b.enabled = source >= 0 && !Muun::Get().pending;
        b.onClick = [this,source,index=recipe.index,name=recipe.name] { Confirm(8, source, index, L"Exchange materials for " + StringUtils::NarrowToWide(name) + L"? This cannot be undone."); };
        menu.buttons.push_back(std::move(b));
    }
    if (menu.buttons.empty()) menu.lines.push_back({L"No exchanges configured on this server."});
    GenericMenuConfig::MenuButton cancel; cancel.label = L"Cancel"; cancel.dismiss = true; menu.buttons.push_back(std::move(cancel));
    g_pGenericMenuDialog->Show(std::move(menu));
}
bool CMuunInventoryWindow::Update() {
    if (!IsVisible()) return true;
    if (SceneFlag != MAIN_SCENE || !g_pNewUISystem->IsVisible(INTERFACE_INVENTORY)) { Show(false); return true; }
    if (m_revision != Muun::Get().revision) { m_dragSlot = -1; m_revision = Muun::Get().revision; Rebuild(); }
    auto& pending = m_view.GetModel().pending;
    if (pending != Muun::Get().pending) { pending = Muun::Get().pending; m_view.MarkDirty("pending"); }
    return true;
}
bool CMuunInventoryWindow::UpdateMouseEvent() {
    if (!IsVisible()) return true;
    auto* doc = m_view.Document(); if (!doc || !UI::RmlBridge::IsPointerWithin(doc->GetElementById("panel"))) return true;
    if (IsRelease(VK_LBUTTON)) DropBagItem({g_fWindowMouseX, g_fWindowMouseY});
    return false;
}
int CMuunInventoryWindow::SlotAt(Rml::Vector2f point) const {
    auto* doc = m_view.Document();
    if (!doc) return -1;
    const auto& entries = m_view.GetModel().items;
    for (int position = 0; position < static_cast<int>(entries.size()); ++position) {
        if (UI::RmlBridge::IsPointWithin(doc->GetElementById("muun_slot_" + std::to_string(position)), point)) {
            constexpr int StoredPageSize = PageSize - 2;
            return position < 2 ? position : 2 + m_page * StoredPageSize + position - 2;
        }
    }
    return -1;
}
void CMuunInventoryWindow::BeginDrag(int position) {
    const auto& state = Muun::Get();
    if (!IsVisible() || state.pending || CInventoryCtrl::GetPickedItem() || position < 0 || position >= PageSize) return;
    const int index = m_visible[position];
    if (index < 0 || index >= static_cast<int>(state.items.size())) return;
    auto* doc = m_view.Document();
    auto* image = doc ? doc->GetElementById("muun_item_" + std::to_string(position)) : nullptr;
    Rml::Vector2f offset;
    if (!image || !UI::RmlBridge::DrawnContentBox(*image, offset, m_dragSize)) return;
    const auto& item = state.items[index];
    m_dragSlot = item.slot; m_dragType = item.type; m_dragLevel = item.level; m_dragRevision = state.revision;
}
void CMuunInventoryWindow::EndDrag(Rml::Vector2f point) {
    const int source = m_dragSlot;
    m_dragSlot = -1;
    const auto& state = Muun::Get();
    if (source < 0 || !IsVisible() || SceneFlag != MAIN_SCENE || state.pending || state.revision != m_dragRevision) return;
    const int target = SlotAt(point);
    if (target >= 0) {
        if (const auto request = Muun::RequestForDrop(state, false, source, m_dragType, target))
            Muun::RequestDrop(*request);
        return;
    }
    auto* doc = m_view.Document();
    if (doc && UI::RmlBridge::IsPointWithin(doc->GetElementById("panel"), point)) return;
    for (auto* bag : {static_cast<CObject*>(g_pMyInventory), static_cast<CObject*>(g_pMyInventoryExt)}) {
        auto* bagDoc = bag && bag->IsVisible() ? bag->GetPlacedDocument() : nullptr;
        if (bagDoc && UI::RmlBridge::IsPointWithin(bagDoc->GetElementById("panel"), point)) {
            Muun::Request(2, source);
            return;
        }
    }
}
void CMuunInventoryWindow::DropBagItem(Rml::Vector2f point) {
    auto* picked = CInventoryCtrl::GetPickedItem();
    if (!IsVisible() || !picked || picked->GetSourceStorageType() != STORAGE_TYPE::INVENTORY || !picked->GetItem()) return;
    const int target = SlotAt(point);
    if (target < 0) return;
    if (const auto request = Muun::RequestForDrop(Muun::Get(), true, picked->GetSourceLinealPos(), picked->GetItem()->Type, target))
        Muun::RequestDrop(*request);
    CInventoryCtrl::BackupPickedItem();
}
void CMuunInventoryWindow::RenderDraggedItem() {
    if (!IsDraggingItem()) return;
    const auto pointer = CInventoryCtrl::PointerPos();
    const Render::Items::ScopedItemEffects effects(false);
    RenderItem3D(pointer.x - m_dragSize.x / 2, pointer.y - m_dragSize.y / 2, m_dragSize.x, m_dragSize.y,
                 m_dragType, m_dragLevel << 3, 0, 0, true);
}
bool CMuunInventoryWindow::UpdateKeyEvent() { if (!IsVisible()) return true; if (IsPress(VK_ESCAPE)) Show(false); return false; }
void CMuunInventoryWindow::DrawItem(int slot, const Rml::Vector2f& p, const Rml::Vector2f& s) {
    const auto& items = Muun::Get().items; const int i = m_visible[slot]; if (i < 0 || i >= static_cast<int>(items.size())) return;
    const Render::Items::ScopedItemEffects effects(false);
    RenderItem3DWithHover(p.x,p.y,s.x,s.y,items[i].type,items[i].level << 3,0,0,false);
}
bool CMuunInventoryWindow::Render() {
    auto* doc = m_view.Document(); if (!doc) return true;
    const auto& items = Muun::Get().items;
    for (int i = 0; i < PageSize; ++i) {
        const int index = m_visible[i];
        const bool shown = IsVisible() && index >= 0 && index < static_cast<int>(items.size()) && items[index].slot != m_dragSlot;
        m_targets[i]->Sync(doc->GetElementById("muun_item_" + std::to_string(i)), shown);
    }
    return true;
}
}

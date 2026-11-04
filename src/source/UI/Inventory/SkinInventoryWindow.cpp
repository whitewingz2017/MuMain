#include "stdafx.h"
#include "UI/Inventory/SkinInventoryWindow.h"
#include "UI/Inventory/SkinShopWindow.h"
#include "UI/Inventory/InventoryCtrl.h"
#include "UI/Core/WindowManager.h"
#include "UI/Core/WindowSystem.h"
#include "UI/RmlBridge/RmlPointer.h"
#include "GameLogic/Cosmetics/CosmeticArmor.h"
#include "Core/Utilities/StringUtils.h"
#include "Core/Input/KeyState.h"
#include "Engine/Object/ZzzInventory.h"
#include "Render/Items/ScopedItemEffects.h"
#include <RmlUi/Core.h>
namespace mu::ui::window {
namespace Skins = GameLogic::Cosmetics;
CSkinInventoryWindow* g_pSkinInventoryWindow = nullptr;
CSkinInventoryWindow::~CSkinInventoryWindow() { Release(); }
void CSkinInventoryWindow::Create(CManager* manager) {
    m_manager = manager;
    m_visible.fill(-1);
    for (int i = 0; i < PageSize; ++i) {
        m_targets[i] = std::make_unique<UI::Items::ItemCameraTarget>([this, i](const auto& p, const auto& s) { DrawItem(i, p, s); });
        m_targets[i]->SetContinuous(false);
    }
    m_view.Ensure();
    manager->AddUIObj(INTERFACE_SKIN_INVENTORY, this);
    Show(false);
}
void CSkinInventoryWindow::Release() {
    Show(false);
    m_view.Release();
    for (auto& target : m_targets) target.reset();
    if (m_manager) m_manager->RemoveUIObj(this);
    m_manager = nullptr;
}
void CSkinInventoryWindow::Bind(Rml::DataModelConstructor& c, Model& m) {
    auto entry = c.RegisterStruct<Entry>();
    entry.RegisterMember("index", &Entry::index);
    entry.RegisterMember("label", &Entry::label);
    c.RegisterArray<std::vector<Entry>>();
    c.Bind("items", &m.items);
    c.Bind("title", &m.title); c.Bind("has_title", &m.hasTitle);
    c.Bind("positioned", &m.positioned); c.Bind("root_x", &m.rootX); c.Bind("root_y", &m.rootY);
    c.Bind("page", &m.page); c.Bind("status", &m.status); c.Bind("pending", &m.pending);
    c.BindEventCallback("storage_close", [this](auto, auto&, const auto&) { Show(false); });
    c.BindEventCallback("storage_category", [this](auto, auto&, const Rml::VariantList& a) {
        if (a.size() != 1) return;
        m_category = std::clamp(a[0].Get<int>(0), 0, 4); m_page = 0; Rebuild();
    });
    c.BindEventCallback("storage_page", [this](auto, auto&, const Rml::VariantList& a) {
        if (a.size() != 1) return;
        const int delta = a[0].Get<int>(0);
        if (delta == -1 || delta == 1) { m_page = std::max(0, m_page + delta); Rebuild(); }
    });
    c.BindEventCallback("storage_remove", [this](auto, auto&, const Rml::VariantList& a) {
        if (a.size() != 1 || Skins::Pending()) return;
        const int index = a[0].Get<int>(-1);
        if (std::find(m_visible.begin(), m_visible.end(), index) != m_visible.end()
            && index >= 0 && index < static_cast<int>(Skins::StoredSkins().size()))
            Skins::Request(4, Skins::StoredSkins()[index].slot);
    });
}
void CSkinInventoryWindow::Open() {
    if (!Skins::Ready()) return;
    if (g_pSkinShopWindow) g_pSkinShopWindow->Show(false);
    g_pNewUISystem->Show(INTERFACE_INVENTORY);
    m_view.Ensure();
    m_page = 0; m_category = 0;
    Show(true); Rebuild(); Skins::Request(0);
}
void CSkinInventoryWindow::Show(bool visible) {
    CObject::Show(visible);
    if (visible && m_view.Document()) m_view.Document()->Show();
    else m_view.Hide();
    if (!visible) {
        for (auto& target : m_targets) if (target) target->Disable();
    }
}
void CSkinInventoryWindow::Rebuild() {
    auto& m = m_view.GetModel();
    std::vector<int> filtered;
    const auto skins = Skins::StoredSkins();
    for (int i = 0; i < static_cast<int>(skins.size()); ++i) {
        const int part = Skins::ArmorPart(skins[i].type);
        const int category = part == 1 ? 1 : part == 7 ? 2 : part == 8 || Skins::IsFenrirMount(skins[i].type) ? 4 : 3;
        if (m_category == 0 || category == m_category) filtered.push_back(i);
    }
    const int pages = std::max(1, (static_cast<int>(filtered.size()) + PageSize - 1) / PageSize);
    m_page = std::clamp(m_page, 0, pages - 1);
    m.items.clear(); m_visible.fill(-1);
    for (int i = m_page * PageSize; i < std::min(static_cast<int>(filtered.size()), (m_page + 1) * PageSize); ++i) {
        const int index = filtered[i];
        m_visible[m.items.size()] = index;
        m.items.push_back({index, StringUtils::WideToNarrow(ItemAttribute[skins[index].type].Name)});
    }
    for (auto& target : m_targets) if (target) target->Invalidate();
    m.page = std::to_string(m_page + 1) + " / " + std::to_string(pages);
    m.status = Skins::StorageSupported() ? "Drag a skin from your bag into this window. Remove returns it to your bag."
        : "Skin storage requires the updated game server.";
    for (const char* field : {"items", "page", "status"}) m_view.MarkDirty(field);
}
bool CSkinInventoryWindow::Update() {
    if (!IsVisible()) return true;
    if (!Skins::Ready() || !g_pNewUISystem->IsVisible(INTERFACE_INVENTORY)) { Show(false); return true; }
    if (m_revision != Skins::Revision()) { m_revision = Skins::Revision(); Rebuild(); }
    auto& pending = m_view.GetModel().pending;
    if (pending != Skins::Pending()) { pending = Skins::Pending(); m_view.MarkDirty("pending"); }
    return true;
}
bool CSkinInventoryWindow::UpdateMouseEvent() {
    if (!IsVisible()) return true;
    auto* doc = m_view.Document();
    if (!doc || !UI::RmlBridge::IsPointerWithin(doc->GetElementById("panel"))) return true;
    auto* picked = CInventoryCtrl::GetPickedItem();
    if (picked && IsRelease(VK_LBUTTON) && picked->GetSourceStorageType() == STORAGE_TYPE::INVENTORY) {
        if (!Skins::Pending()) Skins::Request(3, picked->GetSourceLinealPos());
        CInventoryCtrl::BackupPickedItem();
    }
    return false;
}
bool CSkinInventoryWindow::UpdateKeyEvent() {
    if (!IsVisible()) return true;
    if (IsPress(VK_ESCAPE)) Show(false);
    return false;
}
void CSkinInventoryWindow::DrawItem(int slot, const Rml::Vector2f& p, const Rml::Vector2f& s) {
    const auto skins = Skins::StoredSkins(); const int i = m_visible[slot];
    if (i < 0 || i >= static_cast<int>(skins.size())) return;
    const Render::Items::ScopedItemEffects effects(false);
    RenderItem3DWithHover(p.x, p.y, s.x, s.y, skins[i].type, skins[i].level << 3, skins[i].flags, 0, false);
}
bool CSkinInventoryWindow::Render() {
    auto* doc = m_view.Document(); if (!doc) return true;
    for (int i = 0; i < PageSize; ++i) m_targets[i]->Sync(doc->GetElementById("stored_skin_" + std::to_string(i)), IsVisible() && m_visible[i] >= 0);
    return true;
}
}

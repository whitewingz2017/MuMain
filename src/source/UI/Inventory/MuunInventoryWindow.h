#pragma once
#include "UI/Core/WindowObject.h"
#include "UI/RmlBridge/RmlThemedView.h"
#include "UI/RmlBridge/RmlStackingOrder.h"
#include "UI/Inventory/ItemCameraTarget.h"
#include <array>
#include <memory>
#include <vector>
namespace mu::ui::window {
class CManager;
class CMuunInventoryWindow : public CObject {
public:
    ~CMuunInventoryWindow() override;
    void Create(CManager* manager);
    void Release();
    void Open();
    void Show(bool visible) override;
    bool Update() override;
    bool Render() override;
    bool UpdateMouseEvent() override;
    bool UpdateKeyEvent() override;
    bool IsDraggingItem() const { return IsVisible() && m_dragSlot >= 0; }
    void RenderDraggedItem();
    float GetKeyEventOrder() override { return 100.f; }
    float GetLayerDepth() override { return UI::RmlBridge::SkinShopLayerDepth; }
private:
    static constexpr int PageSize = 22;
    struct Entry { int index = -1, position = 0; Rml::String label; };
    struct Model {
        Rml::String title = "Muun Inventory";
        bool pending = false;
        Rml::String page, status;
        std::vector<Entry> items;
    };
    void Bind(Rml::DataModelConstructor&, Model&);
    void Rebuild();
    int SlotAt(Rml::Vector2f point) const;
    void BeginDrag(int position);
    void EndDrag(Rml::Vector2f point);
    void DropBagItem(Rml::Vector2f point);
    void ShowBook(int page = 0);
    void ShowExchanges();
    void Confirm(int operation, int source, int target, const std::wstring& text);
    void DrawItem(int slot, const Rml::Vector2f&, const Rml::Vector2f&);
    CManager* m_manager = nullptr;
    int m_page = 0;
    std::uint64_t m_revision = 0;
    int m_dragSlot = -1, m_dragType = -1, m_dragLevel = 0;
    std::uint64_t m_dragRevision = 0;
    Rml::Vector2f m_dragSize;
    std::array<int, PageSize> m_visible{};
    std::array<std::unique_ptr<UI::Items::ItemCameraTarget>, PageSize> m_targets;

    UI::RmlBridge::ThemedView<Model> m_view{"muun_inventory",
        [this](Rml::DataModelConstructor& c, Model& m) { Bind(c, m); },
        {{"Data/Interface/RmlUi/muun_inventory.rml"}},
        {.beforeUnload = [this] { m_dragSlot = -1; }}};
};
extern CMuunInventoryWindow* g_pMuunInventoryWindow;
}

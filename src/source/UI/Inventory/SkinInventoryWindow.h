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
class CSkinInventoryWindow : public CObject {
public:
    ~CSkinInventoryWindow() override;
    void Create(CManager* manager);
    void Release();
    void Open();
    void Show(bool visible) override;
    bool Update() override;
    bool Render() override;
    bool UpdateMouseEvent() override;
    bool UpdateKeyEvent() override;
    float GetKeyEventOrder() override { return 100.f; }
    float GetLayerDepth() override { return UI::RmlBridge::SkinShopLayerDepth; }
private:
    static constexpr int PageSize = 6;
    struct Entry { int index = -1; Rml::String label; };
    struct Model {
        Rml::String title = "Skin Inventory";
        bool hasTitle = true, positioned = false, pending = false;
        float rootX = 0, rootY = 0;
        Rml::String page, status;
        std::vector<Entry> items;
    };
    void Bind(Rml::DataModelConstructor&, Model&);
    void Rebuild();
    void DrawItem(int slot, const Rml::Vector2f&, const Rml::Vector2f&);
    CManager* m_manager = nullptr;
    int m_page = 0, m_category = 0;
    std::uint64_t m_revision = 0;
    std::array<int, PageSize> m_visible{};
    std::array<std::unique_ptr<UI::Items::ItemCameraTarget>, PageSize> m_targets;
    UI::RmlBridge::ThemedView<Model> m_view{"skin_inventory",
        [this](Rml::DataModelConstructor& c, Model& m) { Bind(c, m); },
        {{"Data/Interface/RmlUi/skin_inventory.rml"}}};
};
extern CSkinInventoryWindow* g_pSkinInventoryWindow;
}

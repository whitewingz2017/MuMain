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
class CExcellentCombinationWindow : public CObject {
public:
    ~CExcellentCombinationWindow() override;
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
    struct Model {
        Rml::String title="Excellent Option Combination", status="Drag items from your bag onto the two input slots.", cost="Required Zen: 1,000,000";
        bool hasTitle=true, positioned=false, canCombine=false;
        float rootX=0, rootY=0;
    };
    void Bind(Rml::DataModelConstructor&, Model&);
    void Select(int slot, int inventorySlot, ITEM* item);
    void DrawItem(int slot, const Rml::Vector2f&, const Rml::Vector2f&);
    CManager* m_manager=nullptr;
    int m_inspected=-1;
    std::array<int,2> m_slots{-1,-1};
    std::array<ITEM,3> m_items{};
    std::array<bool,3> m_present{};
    std::array<std::unique_ptr<UI::Items::ItemCameraTarget>,3> m_targets;
    std::uint64_t m_revision=0;
    UI::RmlBridge::ThemedView<Model> m_view{"excellent_combination",
        [this](Rml::DataModelConstructor& c, Model& m) { Bind(c,m); },
        {{"Data/Interface/RmlUi/excellent_combination.rml"}}};
};
extern CExcellentCombinationWindow* g_pExcellentCombinationWindow;
}

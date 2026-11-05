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
    struct OptionRow {
        Rml::String text;
        bool potential=false;
    };
    struct Model {
        Rml::String title="Set Stacking", status="Drag an Excellent item from your bag onto Main Gear.", cost;
        std::vector<OptionRow> options;
        bool hasTitle=true, positioned=false, canCombine=false;
        bool hasPrimary=false, hasSecondary=false, affordable=false;
        int activeSlot=0;
        float rootX=0, rootY=0;
    };
    void Bind(Rml::DataModelConstructor&, Model&);
    void Select(int slot, int inventorySlot, ITEM* item);
    bool ValidateSelection();
    void SyncSelection();
    void SyncState();
    void SyncCost();
    void AppendOptions(const ITEM& item, bool potential);
    void DrawItem(int slot, const Rml::Vector2f&, const Rml::Vector2f&);
    CManager* m_manager=nullptr;
    int m_inspected=-1;
    std::array<int,2> m_slots{-1,-1};
    std::array<ITEM,2> m_items{};
    std::array<bool,2> m_present{};
    std::array<std::unique_ptr<UI::Items::ItemCameraTarget>,2> m_targets;
    std::uint64_t m_revision=0;
    std::uint64_t m_gold=static_cast<std::uint64_t>(-1);
    int m_cost=-1;
    UI::RmlBridge::ThemedView<Model> m_view{"excellent_combination",
        [this](Rml::DataModelConstructor& c, Model& m) { Bind(c,m); },
        {{"Data/Interface/RmlUi/excellent_combination.rml"}}};
};
extern CExcellentCombinationWindow* g_pExcellentCombinationWindow;
}

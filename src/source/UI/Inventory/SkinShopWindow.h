#pragma once

#include "UI/Core/WindowObject.h"
#include "UI/RmlBridge/RmlThemedView.h"
#include "UI/RmlBridge/RmlStackingOrder.h"
#include "UI/Inventory/ItemCameraTarget.h"
#include "UI/Inventory/SkinShopPreview.h"
#include <array>
#include <memory>
#include <vector>

namespace mu::ui::window
{
class CManager;
class CSkinShopWindow : public CObject
{
public:
    ~CSkinShopWindow() override;
    void Create(CManager* manager);
    void Release();
    void Open();
    void Show(bool visible) override;
    bool Update() override;
    bool Render() override;
    bool UpdateMouseEvent() override { return !IsVisible(); }
    bool UpdateKeyEvent() override;
    float GetKeyEventOrder() override { return 100.f; }
    float GetLayerDepth() override { return UI::RmlBridge::SkinShopLayerDepth; }

private:
    static constexpr int PageSize = 6;
    struct Entry
    {
        int index = -1;
        Rml::String label;
        Rml::String price;
        bool selected = false;
    };
    struct Model
    {
        Rml::String title = "Skin Shop";
        bool hasTitle = true;
        bool positioned = false;
        float rootX = 0;
        float rootY = 0;
        Rml::String balance;
        Rml::String status;
        Rml::String selectedName = "Select a skin";
        Rml::String price;
        Rml::String note;
        Rml::String page;
        int category = 0;
        int previewLevel = 0;
        int openDropdown = 0;
        Rml::String bonusLabel = "Choose bonus";
        Rml::String weaponLabel = "All weapons";
        std::vector<Entry> weaponTypes;
        bool pending = false;
        bool canBuy = false;
        bool previous = false;
        bool next = false;
        std::vector<Entry> items;
        std::vector<Entry> bonuses;
    };
    void Bind(Rml::DataModelConstructor& constructor, Model& model);
    void BindActions(Rml::DataModelConstructor& constructor);
    void Load();
    void RebuildItems();
    void Select(int index);
    void SelectBonus(int number);
    void SetPreviewLevel(int delta);
    void UpdateDetails();
    void Buy();
    void SyncTargets();
    void DrawItem(int slot, const Rml::Vector2f& offset, const Rml::Vector2f& size);

    CManager* m_manager = nullptr;
    int m_page = 0;
    int m_selected = -1;
    int m_bonus = 0;
    int m_weaponFilter = 0;
    unsigned m_revision = 0;
    std::array<int, PageSize> m_visible{};
    std::array<std::unique_ptr<UI::Items::ItemCameraTarget>, PageSize> m_itemTargets;
    std::unique_ptr<UI::Inventory::SkinShopPreview> m_preview;
    UI::RmlBridge::ThemedView<Model> m_view{"skin_shop",
        [this](Rml::DataModelConstructor& constructor, Model& model) { Bind(constructor, model); },
        {{"Data/Interface/RmlUi/skin_shop.rml"}},
        {.modal = Rml::ModalFlag::Modal, .focus = Rml::FocusFlag::Document}};
};
extern CSkinShopWindow* g_pSkinShopWindow;
}

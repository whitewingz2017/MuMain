#pragma once

#include "UI/Core/WindowObject.h"
#include "UI/RmlBridge/RmlStackingOrder.h"
#include "UI/RmlBridge/RmlThemedView.h"

#include <vector>

namespace mu::ui::window
{
class CManager;

// The rank title shop: every title of RankTitles.json with the server's price, bought with WCoinC,
// and the owned titles, one of which the character shows (docs/rank-titles.md).
class CRankShopWindow : public CObject
{
public:
    ~CRankShopWindow() override;
    void Create(CManager* manager);
    void Release();
    void Open();
    void Show(bool visible) override;
    bool Update() override;
    bool Render() override { return true; }
    bool UpdateMouseEvent() override { return !IsVisible(); }
    bool UpdateKeyEvent() override;
    float GetKeyEventOrder() override { return 100.f; }
    float GetLayerDepth() override { return UI::RmlBridge::SkinShopLayerDepth; }

private:
    enum Tab
    {
        ShopTab = 0,
        OwnedTab = 1,
    };
    struct Card
    {
        int rank = 0;
        Rml::String image;
        Rml::String label;
        Rml::String detail;
        bool owned = false;
        bool active = false;
        bool selected = false;
    };
    struct Model
    {
        Rml::String title = "Rank Shop";
        bool hasTitle = true;
        bool positioned = false;
        float rootX = 0;
        float rootY = 0;
        Rml::String balance;
        Rml::String status;
        int tab = ShopTab;
        std::vector<Card> cards;
        Rml::String page;
        bool previous = false;
        bool next = false;
        bool pending = false;
        Rml::String selectedImage;
        Rml::String selectedLabel;
        Rml::String selectedDetail;
        Rml::String actionLabel;
        bool canAct = false;
    };
    void Bind(Rml::DataModelConstructor& constructor, Model& model);
    void BindActions(Rml::DataModelConstructor& constructor);
    void Load();
    void SelectTab(int tab);
    void TurnPage(int delta);
    void Select(int rank);
    void Act();
    void SyncShop();
    void RebuildCards();
    void UpdateSelection();
    Card MakeCard(int rank) const;
    std::vector<int> ListedRanks() const;

    CManager* m_manager = nullptr;
    int m_page = 0;
    int m_selectedRank = 0;
    unsigned m_revision = 0;
    UI::RmlBridge::ThemedView<Model> m_view{"rank_shop",
        [this](Rml::DataModelConstructor& constructor, Model& model) { Bind(constructor, model); },
        {{"Data/Interface/RmlUi/rank_shop.rml"}},
        {.modal = Rml::ModalFlag::Modal, .focus = Rml::FocusFlag::Document}};
};
extern CRankShopWindow* g_pRankShopWindow;
} // namespace mu::ui::window

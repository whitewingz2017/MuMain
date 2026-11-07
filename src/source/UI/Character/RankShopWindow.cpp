#include "stdafx.h"
#include "UI/Character/RankShopWindow.h"

#include "Core/Utilities/StringUtils.h"
#include "Data/GameData/RankTitles/RankTitles.h"
#include "GameLogic/RankTitles/RankShop.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Scenes/SceneCore.h"
#include "UI/Core/WindowCommon.h"
#include "UI/Core/WindowManager.h"

#include <RmlUi/Core.h>

#include <algorithm>
#include <iterator>
#include <string>

namespace mu::ui::window
{
namespace
{
namespace Shop = GameLogic::RankTitles::Shop;

// Two columns of six, as the shop pages its titles.
constexpr int PageSize = 12;
constexpr const char* Currency = " WCoinC";
// A title the server has no offer for (or hasn't answered for yet).
constexpr const char* NoPrice = "-";

// The server's offer for the rank; nullptr when it sells none.
const Shop::Offer* FindOffer(int rank)
{
    const auto offers = Shop::Offers();
    const auto found = std::find_if(offers.begin(), offers.end(),
                                    [rank](const Shop::Offer& offer) { return offer.rank == rank; });
    return found != offers.end() ? &*found : nullptr;
}

bool IsOwned(int rank)
{
    const auto* offer = FindOffer(rank);
    return offer && offer->owned;
}

Rml::String TitleName(int rank)
{
    const auto* offer = FindOffer(rank);
    if (offer && !offer->name.empty())
        return StringUtils::WideToNarrow(offer->name.c_str());
    return "Rank " + std::to_string(rank);
}

// RmlUi reads "/Data/..." from the client folder; the .tga name reads its packed .OZT.
Rml::String BadgeSource(int rank)
{
    const auto* title = Data::RankTitles::Find(rank);
    return title && !title->badge.image.empty() ? "/" + title->badge.image : Rml::String();
}
} // namespace

CRankShopWindow* g_pRankShopWindow = nullptr;

CRankShopWindow::~CRankShopWindow()
{
    Release();
}

void CRankShopWindow::Create(CManager* manager)
{
    m_manager = manager;
    m_view.Ensure();
    manager->AddUIObj(INTERFACE_RANK_SHOP, this);
    Show(false);
}

void CRankShopWindow::Release()
{
    Show(false);
    m_view.Release();
    if (m_manager)
        m_manager->RemoveUIObj(this);
    m_manager = nullptr;
}

void CRankShopWindow::Bind(Rml::DataModelConstructor& constructor, Model& model)
{
    auto card = constructor.RegisterStruct<Card>();
    card.RegisterMember("rank", &Card::rank);
    card.RegisterMember("image", &Card::image);
    card.RegisterMember("label", &Card::label);
    card.RegisterMember("detail", &Card::detail);
    card.RegisterMember("owned", &Card::owned);
    card.RegisterMember("active", &Card::active);
    card.RegisterMember("selected", &Card::selected);
    constructor.RegisterArray<std::vector<Card>>();
    constructor.Bind("cards", &model.cards);
    constructor.Bind("title", &model.title);
    constructor.Bind("has_title", &model.hasTitle);
    constructor.Bind("positioned", &model.positioned);
    constructor.Bind("root_x", &model.rootX);
    constructor.Bind("root_y", &model.rootY);
    constructor.Bind("balance", &model.balance);
    constructor.Bind("status", &model.status);
    constructor.Bind("tab", &model.tab);
    constructor.Bind("page", &model.page);
    constructor.Bind("previous", &model.previous);
    constructor.Bind("next", &model.next);
    constructor.Bind("pending", &model.pending);
    constructor.Bind("selected_image", &model.selectedImage);
    constructor.Bind("selected_label", &model.selectedLabel);
    constructor.Bind("selected_detail", &model.selectedDetail);
    constructor.Bind("action_label", &model.actionLabel);
    constructor.Bind("can_act", &model.canAct);
    BindActions(constructor);
}

void CRankShopWindow::BindActions(Rml::DataModelConstructor& constructor)
{
    const auto argument = [](const Rml::VariantList& args) { return args.size() == 1 ? args[0].Get<int>(-1) : -1; };
    constructor.BindEventCallback("rank_close", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) {
        Show(false);
    });
    constructor.BindEventCallback("rank_refresh", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) {
        Load();
    });
    constructor.BindEventCallback("rank_action", [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList&) {
        Act();
    });
    constructor.BindEventCallback("rank_tab", [this, argument](Rml::DataModelHandle, Rml::Event&,
                                                               const Rml::VariantList& args) { SelectTab(argument(args)); });
    constructor.BindEventCallback("rank_page", [this, argument](Rml::DataModelHandle, Rml::Event&,
                                                                const Rml::VariantList& args) { TurnPage(argument(args)); });
    constructor.BindEventCallback("rank_select", [this, argument](Rml::DataModelHandle, Rml::Event&,
                                                                  const Rml::VariantList& args) { Select(argument(args)); });
}

void CRankShopWindow::Open()
{
    if (SceneFlag != MAIN_SCENE || !Hero)
        return;
    m_view.Ensure();
    Show(true);
    // The titles show at once, from RankTitles.json; the server's answer adds prices and ownership.
    m_revision = Shop::Revision();
    SyncShop();
    Load();
}

void CRankShopWindow::Show(bool visible)
{
    CObject::Show(visible);
    if (visible && m_view.Document())
        m_view.Document()->Show(Rml::ModalFlag::Modal, Rml::FocusFlag::Document);
    else
        m_view.Hide();
}

void CRankShopWindow::Load()
{
    if (Shop::Pending() || !Shop::Load())
        return;
    m_view.GetModel().status = "Loading rank titles...";
    m_view.MarkDirty("status");
}

void CRankShopWindow::SelectTab(int tab)
{
    if ((tab != ShopTab && tab != OwnedTab) || Shop::Pending())
        return;
    m_view.GetModel().tab = tab;
    m_view.MarkDirty("tab");
    m_page = 0;
    m_selectedRank = 0;
    RebuildCards();
}

void CRankShopWindow::TurnPage(int delta)
{
    const auto& model = m_view.GetModel();
    const bool allowed = delta == -1 ? model.previous : delta == 1 && model.next;
    if (!allowed || Shop::Pending())
        return;
    m_page += delta;
    RebuildCards();
}

void CRankShopWindow::Select(int rank)
{
    const auto& cards = m_view.GetModel().cards;
    const bool listed = std::any_of(cards.begin(), cards.end(), [rank](const Card& card) { return card.rank == rank; });
    if (!listed || Shop::Pending())
        return;
    m_selectedRank = rank;
    for (auto& card : m_view.GetModel().cards)
        card.selected = card.rank == rank;
    m_view.MarkDirty("cards");
    UpdateSelection();
}

// The ranks the tab lists, in RankTitles.json's order: every title, or the owned ones.
std::vector<int> CRankShopWindow::ListedRanks() const
{
    const auto& ranks = Data::RankTitles::Ranks();
    if (m_view.GetModel().tab != OwnedTab)
        return ranks;
    std::vector<int> owned;
    std::copy_if(ranks.begin(), ranks.end(), std::back_inserter(owned), IsOwned);
    return owned;
}

CRankShopWindow::Card CRankShopWindow::MakeCard(int rank) const
{
    const auto* offer = FindOffer(rank);
    Card card;
    card.rank = rank;
    card.image = BadgeSource(rank);
    card.label = TitleName(rank);
    card.owned = offer && offer->owned;
    card.active = rank == Shop::Active();
    card.selected = rank == m_selectedRank;
    if (m_view.GetModel().tab == OwnedTab)
        card.detail = card.active ? "Showing" : "Owned";
    else if (!offer)
        card.detail = NoPrice;
    else
        card.detail = offer->owned ? "Owned" : std::to_string(offer->price) + Currency;
    return card;
}

void CRankShopWindow::RebuildCards()
{
    auto& model = m_view.GetModel();
    const auto listed = ListedRanks();
    const int count = static_cast<int>(listed.size());
    m_page = std::clamp(m_page, 0, std::max(0, (count - 1) / PageSize));
    model.cards.clear();
    for (int slot = m_page * PageSize; slot < std::min(count, (m_page + 1) * PageSize); ++slot)
        model.cards.push_back(MakeCard(listed[slot]));
    // A title that left the list (sold out of the tab, or gone from the shop) is no longer selected.
    if (std::none_of(model.cards.begin(), model.cards.end(), [this](const Card& card) { return card.rank == m_selectedRank; }))
        m_selectedRank = 0;
    model.previous = m_page > 0;
    model.next = count > (m_page + 1) * PageSize;
    model.page = std::to_string(m_page + 1) + " / " + std::to_string(std::max(1, (count + PageSize - 1) / PageSize));
    for (const char* field : {"cards", "previous", "next", "page"})
        m_view.MarkDirty(field);
    UpdateSelection();
}

void CRankShopWindow::UpdateSelection()
{
    auto& model = m_view.GetModel();
    const auto* offer = FindOffer(m_selectedRank);
    const auto balance = Shop::Balance();
    const bool selected = m_selectedRank != 0;
    model.selectedImage = selected ? BadgeSource(m_selectedRank) : Rml::String();
    model.selectedLabel = selected ? TitleName(m_selectedRank) : "Select a title";
    model.selectedDetail.clear();
    model.actionLabel = model.tab == OwnedTab ? "Equip" : "Buy";
    model.canAct = false;
    if (selected && model.tab == OwnedTab)
    {
        const bool showing = m_selectedRank == Shop::Active();
        model.actionLabel = showing ? "Unequip" : "Equip";
        model.selectedDetail = showing ? "Shown over your name." : "Owned. Equip it to show it over your name.";
        model.canAct = !Shop::Pending() && offer && offer->owned;
    }
    else if (selected && !balance)
    {
        model.selectedDetail = "Shop data unavailable.";
    }
    else if (selected && !offer)
    {
        model.selectedDetail = "Not on sale.";
    }
    else if (selected)
    {
        model.actionLabel = "Buy " + std::to_string(offer->price) + Currency;
        model.selectedDetail = offer->owned ? "You own this title: equip it in My Ranks."
                               : offer->price > *balance ? "Not enough WCoinC." : "Permanent.";
        model.canAct = !Shop::Pending() && !offer->owned && offer->price > 0 && offer->price <= *balance;
    }
    for (const char* field : {"selected_image", "selected_label", "selected_detail", "action_label", "can_act"})
        m_view.MarkDirty(field);
}

void CRankShopWindow::Act()
{
    const auto* offer = FindOffer(m_selectedRank);
    if (!m_view.GetModel().canAct || !offer)
        return;
    const bool sent = m_view.GetModel().tab == OwnedTab ? Shop::Equip(offer->rank == Shop::Active() ? 0 : offer->rank)
                                                        : Shop::Buy(*offer);
    if (!sent)
        return;
    m_view.GetModel().status = "Waiting for the server...";
    m_view.MarkDirty("status");
    UpdateSelection();
}

// After an answer: the balance, the status and the lists, keeping the tab, page and selection.
void CRankShopWindow::SyncShop()
{
    auto& model = m_view.GetModel();
    const auto balance = Shop::Balance();
    model.balance = balance ? "WCoinC: " + std::to_string(*balance) : "WCoinC: Unavailable";
    const auto& status = Shop::Status();
    model.status = status.empty() || status == L"Ready" ? "Select a title." : StringUtils::WideToNarrow(status.c_str());
    m_view.MarkDirty("balance");
    m_view.MarkDirty("status");
    RebuildCards();
}

bool CRankShopWindow::Update()
{
    Shop::Update();
    if (!IsVisible())
        return true;
    if (SceneFlag != MAIN_SCENE || !Hero)
    {
        Show(false);
        return true;
    }
    if (m_revision != Shop::Revision())
    {
        m_revision = Shop::Revision();
        SyncShop();
    }
    auto& model = m_view.GetModel();
    if (model.pending != Shop::Pending())
    {
        model.pending = Shop::Pending();
        m_view.MarkDirty("pending");
        UpdateSelection();
    }
    return true;
}

bool CRankShopWindow::UpdateKeyEvent()
{
    if (!IsVisible())
        return true;
    if (IsPress(VK_ESCAPE))
        Show(false);
    return false;
}
} // namespace mu::ui::window

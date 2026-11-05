
#include "stdafx.h"
#include "UI/Inventory/ExcellentCombinationWindow.h"
#include "GameLogic/Events/EventSchedule.h"
#include "GameLogic/Pets/Muun.h"
#include "UI/Inventory/MuunInventoryWindow.h"
#include "UI/Dialogs/WindowMenu.h"
#include "UI/Dialogs/RedeemCodeDialog.h"
#include "UI/Bank/JewelBank.h"
#include "UI/Core/WindowSystem.h"
#include "UI/Core/WindowCommon.h" // ShowSystemMenuDialog
#include "Audio/DSPlaySound.h"
#include "I18N/All.h"

#include "Core/Utilities/StringUtils.h"
#include "Render/RmlUi/RmlUiRuntime.h"
#include "UI/RmlBridge/RmlDocumentVisibility.h"
#include "UI/RmlBridge/RmlNativeTextSize.h"
#include "UI/RmlBridge/RmlTheme.h"

#include <RmlUi/Core/ElementDocument.h>

using namespace SEASON3B;
using namespace mu::ui::window;

namespace
{
// The row labels, top to bottom (CWindowMenu::MenuEntry order).
const wchar_t* const* const kMenuLabels[] = {
    &I18N::Game::SystemEsc, &I18N::Game::HelpF1, &I18N::Game::GuildG,
    &I18N::Game::MoveM,     &I18N::Game::MapTab, &I18N::Game::GensInfoB,
};
// The TwinMU rows follow the original six; the game text tables have no labels for them.
const wchar_t* const kTwinMuLabels[] = {L"Redeem Code", L"Excellent Combination", L"Event Timers", L"Muun Inventory", L"Jewel Bank"};
static_assert(std::size(kMenuLabels) + std::size(kTwinMuLabels) == CWindowMenu::MENU_MAX_INDEX,
              "one label per menu row");

template <typename Model>
void SyncFloat(RmlModelBinder<Model>& binder, float Model::* field, const char* name, float value)
{
    Model& model = binder.GetModel();
    if (model.*field == value)
        return;
    model.*field = value;
    binder.MarkDirty(name);
}
} // namespace

mu::ui::window::CWindowMenu::CWindowMenu()
{
    m_pNewUIMng = NULL;
}

mu::ui::window::CWindowMenu::~CWindowMenu()
{
    Release();
}

bool mu::ui::window::CWindowMenu::Create(CManager* pNewUIMng)
{
    if (NULL == pNewUIMng)
        return false;

    m_pNewUIMng = pNewUIMng;
    m_pNewUIMng->AddUIObj(mu::ui::window::INTERFACE_WINDOW_MENU, this);

    BuildRmlUi();

    Show(false);

    return true;
}

void mu::ui::window::CWindowMenu::Release()
{

    if (m_pNewUIMng)
    {
        m_pNewUIMng->RemoveUIObj(this);
        m_pNewUIMng = NULL;
    }

    m_RmlView.Release();
}

bool mu::ui::window::CWindowMenu::UpdateMouseEvent()
{
    // Row hover and clicks are RmlUi's. Modal: consumes every mouse event while visible so
    // nothing behind it can fire.
    return false;
}

bool mu::ui::window::CWindowMenu::UpdateKeyEvent()
{
    if (g_pNewUISystem->IsVisible(mu::ui::window::INTERFACE_WINDOW_MENU) == true)
    {
        if (mu::ui::window::IsPress(VK_ESCAPE) == true)
        {
            g_pNewUISystem->Hide(mu::ui::window::INTERFACE_WINDOW_MENU);
            PlayBuffer(SOUND_CLICK01);

            return false;
        }
    }

    return true;
}

bool mu::ui::window::CWindowMenu::Update()
{
    GameLogic::Events::Schedule::Update();
    GameLogic::Pets::Muun::Update();
    SyncRmlModel();

    if (m_PendingEntry >= 0)
    {
        const int entry = m_PendingEntry;
        m_PendingEntry = -1;
        if (IsVisible())
            RunMenuEntry(entry);
    }

    return true;
}

bool mu::ui::window::CWindowMenu::Render()
{
    // Nothing native left: frame, rows, hover and arrows are RmlUi. Kept because CObject
    // requires the override.
    return true;
}

void mu::ui::window::CWindowMenu::RunMenuEntry(int entry)
{
    switch (entry)
    {
    case MENU_MUUN:
        g_pNewUISystem->Hide(INTERFACE_WINDOW_MENU);
        if (g_pMuunInventoryWindow) g_pMuunInventoryWindow->Open();
        break;
    case MENU_JEWEL_BANK:
        g_pNewUISystem->Hide(mu::ui::window::INTERFACE_WINDOW_MENU);
        UI::Bank::Open();
        break;
    case MENU_EVENT_TIMERS:
        g_pNewUISystem->Hide(mu::ui::window::INTERFACE_WINDOW_MENU);
        GameLogic::Events::Schedule::Open();
        break;
    case MENU_SYSTEM:
        g_pNewUISystem->Hide(mu::ui::window::INTERFACE_WINDOW_MENU);
        mu::ui::window::ShowSystemMenuDialog();
        break;
    case MENU_HELP:
        if (g_pNewUISystem->IsVisible(mu::ui::window::INTERFACE_HELP))
            g_pHelp->AutoUpdateIndex();
        else
            g_pNewUISystem->Show(mu::ui::window::INTERFACE_HELP);
        break;
    case MENU_GUILD:
        g_pNewUISystem->Show(mu::ui::window::INTERFACE_GUILDINFO);
        g_pNewUISystem->Hide(mu::ui::window::INTERFACE_WINDOW_MENU);
        break;
    case MENU_MOVE:
        g_pNewUISystem->Toggle(mu::ui::window::INTERFACE_MOVEMAP);
        break;
    case MENU_MINIMAP:
        g_pNewUISystem->Hide(mu::ui::window::INTERFACE_WINDOW_MENU);
        if (g_pNewUIMiniMap->m_bSuccess == false)
            g_pNewUISystem->Hide(mu::ui::window::INTERFACE_MINI_MAP);
        else
            g_pNewUISystem->Toggle(mu::ui::window::INTERFACE_MINI_MAP);
        break;
    case MENU_EXCELLENT_COMBINATION:
        g_pNewUISystem->Hide(INTERFACE_WINDOW_MENU);
        if (g_pExcellentCombinationWindow) g_pExcellentCombinationWindow->Open();
        break;
    case MENU_REDEEM:
        g_pNewUISystem->Hide(mu::ui::window::INTERFACE_WINDOW_MENU);
        UI::RedeemCodes::Open();
        break;
    case MENU_GENS:
        if (g_pNewUIGensRanking->SetGensInfo())
        {
            g_pNewUISystem->Show(mu::ui::window::INTERFACE_GENSRANKING);
            g_pNewUISystem->Hide(mu::ui::window::INTERFACE_WINDOW_MENU);
        }
        break;
    default:
        break;
    }
}

void mu::ui::window::CWindowMenu::BindRmlModel(Rml::DataModelConstructor& c, WindowMenuRmlModel& model)
{
    c.Bind("text_px", &model.textPx);

    auto row = c.RegisterStruct<WindowMenuRowEntry>();
    row.RegisterMember("label", &WindowMenuRowEntry::label);
    row.RegisterMember("index", &WindowMenuRowEntry::index);
    c.RegisterArray<std::vector<WindowMenuRowEntry>>();
    c.Bind("rows", &model.rows);

    c.BindEventCallback("windowmenu_select",
                        [this](Rml::DataModelHandle, Rml::Event&, const Rml::VariantList& arguments)
                        {
                            if (arguments.size() == 1)
                                m_PendingEntry = arguments[0].Get<int>(-1);
                        });

    model.rows.clear();
    for (int i = 0; i < MENU_MAX_INDEX; ++i)
    {
        const int original = static_cast<int>(std::size(kMenuLabels));
        const wchar_t* label = i < original ? *kMenuLabels[i] : kTwinMuLabels[i - original];
        model.rows.push_back({StringUtils::WideToNarrow(label), i});
    }
}

void mu::ui::window::CWindowMenu::BuildRmlUi()
{
    m_RmlView.Ensure();
}

void mu::ui::window::CWindowMenu::SyncRmlModel()
{
    BuildRmlUi();
    if (!m_RmlView.Document())
        return;

    // The original drew the menu over the HUD and over every window below its layer depth.
    UI::RmlBridge::SyncDocumentVisibilityInFront(m_RmlView.Document(), IsVisible());
    if (!IsVisible())
        return;

    SyncTransform();
}

void mu::ui::window::CWindowMenu::SyncTransform()
{
    UI::RmlBridge::SyncNativeTextSize(m_RmlView.Binder());
}

float mu::ui::window::CWindowMenu::GetLayerDepth()
{
    return 10.4f;
}

float mu::ui::window::CWindowMenu::GetKeyEventOrder()
{
    return 10.f;
}

void mu::ui::window::CWindowMenu::OpenningProcess()
{
    m_PendingEntry = -1;
}

void mu::ui::window::CWindowMenu::ClosingProcess() {}

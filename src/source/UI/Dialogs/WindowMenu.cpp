
#include "stdafx.h"
#include "UI/Inventory/ExcellentCombinationWindow.h"
#include "GameLogic/Events/EventSchedule.h"
#include "GameLogic/Pets/Muun.h"
#include "UI/Inventory/MuunInventoryWindow.h"
#include "Camera/CameraManager.h"
#include "World/MapInfra/MapManager.h"
#include "UI/Dialogs/WindowMenu.h"
#include "UI/Dialogs/RedeemCodeDialog.h"
#include "UI/Core/WindowSystem.h"
#include "UI/Character/NameWindow.h"
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
constexpr int kMenuTextIds[CWindowMenu::MENU_MAX_INDEX] = {1741, 1742, 0, 0, 0, 0, 0, 0, 0, 0, 364, 1743, 3055, 3103};

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
    case MENU_CHAT:
        g_pNewUISystem->Hide(mu::ui::window::INTERFACE_WINDOW_MENU);
        g_pNewUISystem->Show(mu::ui::window::INTERFACE_PARTY);
        break;
    case MENU_DROP_NAMES:
        if (g_pNameWindow) g_pNameWindow->ToggleDropNames();
        g_pNewUISystem->Hide(mu::ui::window::INTERFACE_WINDOW_MENU);
        break;
    case MENU_CAMERA:
        CameraManager::Instance().SetCameraMode(CameraManager::Instance().GetCurrentMode() == CameraMode::Default ? CameraMode::Orbital : CameraMode::Default);
        break;
    case MENU_CAMERA_RESET:
        CameraManager::Instance().ResetActiveView();
        break;
    case MENU_MONSTER_HP:
        if (g_pNameWindow) g_pNameWindow->ToggleMonsterHealthBars();
        break;
    case MENU_PLAYER_NAMES:
        if (g_pNameWindow) g_pNameWindow->TogglePlayerNames();
        break;
    case MENU_MONSTER_NAMES:
        if (g_pNameWindow) g_pNameWindow->ToggleMonsterNames();
        break;
    case MENU_COMMAND:
        g_pNewUISystem->Hide(INTERFACE_WINDOW_MENU);
        if (!gMapManager.InChaosCastle() && !::IsStrifeMap(gMapManager.WorldActive))
            g_pNewUISystem->Toggle(INTERFACE_COMMAND);
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
        const wchar_t* label = i == MENU_MUUN ? L"Muun Inventory" : i == MENU_EVENT_TIMERS ? L"Event Timers" : i == MENU_EXCELLENT_COMBINATION ? L"Excellent Combination" : i == MENU_REDEEM ? L"Redeem Code" : i == MENU_CHAT ? L"Party" :
            i == MENU_CAMERA ? L"Toggle 3D Camera" :
            i == MENU_CAMERA_RESET ? L"Reset Camera View" :
            i == MENU_MONSTER_HP ? L"Toggle Monster HP Bar" :
            i == MENU_PLAYER_NAMES ? L"Hide Player Names" :
            i == MENU_MONSTER_NAMES ? L"Hide Monster Names" :
            i == MENU_COMMAND ? L"Command (D)" :
            i == MENU_DROP_NAMES ? L"Show Drop Name" : I18N::Game::Lookup(kMenuTextIds[i]);
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

    auto& label = m_RmlView.GetModel().rows[MENU_DROP_NAMES].label;
    const Rml::String text = g_pNameWindow && g_pNameWindow->AreDropNamesShown() ? "Hide Drop Name" : "Show Drop Name";
    if (label != text)
    {
        label = text;
        m_RmlView.MarkDirty("rows");
    }

    const auto setNameLabel = [this](int entry, bool shown, const char* show, const char* hide) {
        auto& value = m_RmlView.GetModel().rows[entry].label;
        const Rml::String text = shown ? hide : show;
        if (value != text) { value = text; m_RmlView.MarkDirty("rows"); }
    };
    setNameLabel(MENU_PLAYER_NAMES, g_pNameWindow && g_pNameWindow->ArePlayerNamesShown(), "Show Player Names", "Hide Player Names");
    setNameLabel(MENU_MONSTER_NAMES, g_pNameWindow && g_pNameWindow->AreMonsterNamesShown(), "Show Monster Names", "Hide Monster Names");
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

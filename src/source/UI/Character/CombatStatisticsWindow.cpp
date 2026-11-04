#include "stdafx.h"
#include "UI/Character/CombatStatisticsWindow.h"
#include "GameLogic/Cosmetics/CosmeticArmor.h"
#include "UI/Core/WindowSystem.h"
#include "UI/RmlBridge/RmlPointer.h"
#include "UI/RmlBridge/RmlWindowClose.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <RmlUi/Core.h>

namespace mu::ui::window
{
CCombatStatisticsWindow::~CCombatStatisticsWindow()
{
    m_view.Release();
    if (m_manager) m_manager->RemoveUIObj(this);
}

void CCombatStatisticsWindow::Create(CManager* manager)
{
    m_manager = manager;
    manager->AddUIObj(INTERFACE_COMBAT_STATISTICS, this);
    m_view.Ensure();
    Show(false);
}

void CCombatStatisticsWindow::Bind(Rml::DataModelConstructor& c, Model& model)
{
    c.RegisterArray<std::vector<Rml::String>>();
    c.Bind("statistics", &model.statistics);
    UI::RmlBridge::BindWindowClose(c, INTERFACE_COMBAT_STATISTICS);
}

void CCombatStatisticsWindow::Show(bool visible)
{
    CObject::Show(visible);
    if (!m_view.Document()) return;
    if (!visible) { m_view.Hide(); return; }
    m_lastRequest = 0;
    Update();
    m_view.Document()->Show();
}

bool CCombatStatisticsWindow::Update()
{
    if (!IsVisible() || !m_view.Ensure()) return true;
    constexpr Uint64 RefreshMs = 1000;
    const Uint64 now = SDL_GetTicks();
    if ((m_lastRequest == 0 || now - m_lastRequest >= RefreshMs) && GameLogic::Cosmetics::Request(0))
        m_lastRequest = now;
    const auto values = GameLogic::Cosmetics::Statistics();
    auto& rows = m_view.GetModel().statistics;
    if (values.empty())
    {
        static const Rml::String waiting = "Waiting for server statistics...";
        if (rows.size() == 1 && rows.front() == waiting) return true;
        rows.assign(1, waiting);
    }
    else
    {
        // A refresh must not reset scroll or allocate rows again while nothing changed.
        if (std::equal(values.begin(), values.end(), rows.begin(), rows.end())) return true;
        rows.assign(values.begin(), values.end());
    }
    m_view.MarkDirty("statistics");
    return true;
}

bool CCombatStatisticsWindow::UpdateMouseEvent()
{
    return !UI::RmlBridge::IsPointerOver(m_view.Document());
}

bool CCombatStatisticsWindow::UpdateKeyEvent()
{
    if (IsVisible() && IsPress(VK_ESCAPE))
    {
        g_pNewUISystem->Hide(INTERFACE_COMBAT_STATISTICS);
        return false;
    }
    return true;
}
}

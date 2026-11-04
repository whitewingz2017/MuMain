
#if !defined(AFX_NEWUINAMEWINDOW_H__76B140FF_46CB_4DB6_9DA2_5F84F294D212__INCLUDED_)
#define AFX_NEWUINAMEWINDOW_H__76B140FF_46CB_4DB6_9DA2_5F84F294D212__INCLUDED_

#pragma once

#include "UI/Core/WindowManager.h"
#include "UI/Character/WorldLabelLayer.h"
#include "UI/HUD/HudStatus.h"
#include "UI/HUD/MatchStatus.h"

namespace mu::ui::window
{
    // item name
    class CNameWindow : public CObject
    {
    public:
        CNameWindow();
        virtual ~CNameWindow();

        bool Create(CManager* pNewUIMng);
        void Release();


        bool UpdateMouseEvent();
        bool UpdateKeyEvent();
        bool Update();
        bool Render();
        void PrepareFrame() override;
        void Show(bool bShow) override;

        // True when the world-label layer draws RenderInterface()'s overlays -- the party members'
        // HP bars over their heads, the siege build-time bars, the Kanturu result banner -- and the
        // Kalima object labels (RenderObjectDescription()), recorded under the name labels as the
        // original drew them before them; the main scene then leaves them out.
        bool RecordsInterfaceOverlays() const;
        bool ArePlayerNamesShown() const { return m_ShowPlayerNames; }
        bool AreMonsterNamesShown() const { return m_ShowMonsterNames; }
        bool IsMonsterNameShown(const CHARACTER* character) const;
        void TogglePlayerNames() { m_ShowPlayerNames = !m_ShowPlayerNames; }
        void ToggleMonsterNames() { m_ShowMonsterNames = !m_ShowMonsterNames; }
        bool AreDropNamesShown() const { return m_bShowItemName; }
        void ToggleMonsterHealthBars() { m_bShowMonsterHealthBar = !m_bShowMonsterHealthBar; }
        bool AreMonsterHealthBarsShown() const { return m_bShowMonsterHealthBar; }
        void ToggleDropNames() { m_bShowItemName = !m_bShowItemName; }

        float GetLayerDepth();		// 1.0f

    private:
        // Everything this window draws; recorded into m_labelLayer (RmlUi) when it is available,
        // drawn natively otherwise.
        void RenderLabels();
        void RenderName();
        void RenderCharacterNames();
        void SyncMonsterTarget();
        void RenderMonsterTarget();
        int m_monsterTargetKey = -1;
        int m_monsterTargetIndex = -1;
        void RenderGroundItemName(int index, bool sorted);

        UI::Character::WorldLabelLayer m_labelLayer;
        UI::Hud::StatusTexts m_statusTexts;
        UI::Hud::MatchStatus m_matchStatus;

        CManager* m_pNewUIMng;		// UI manager


        bool m_ShowPlayerNames = true;
        bool m_ShowMonsterNames = true;
        bool m_bShowItemName;
        bool m_bShowMonsterHealthBar;

        void RenderMonsterHealthBars();
    };
}

#endif // !defined(AFX_NEWUINAMEWINDOW_H__76B140FF_46CB_4DB6_9DA2_5F84F294D212__INCLUDED_)

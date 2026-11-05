///////////////////////////////////////////////////////////////////////////////
// WebzenScene.cpp - Webzen title/intro scene implementation
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Core/Utilities/Random.h"
#include "WebzenScene.h"
#include "SceneCore.h"
#include "Core/Platform/ScopedSystemCursor.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "Engine/Object/ZzzInterface.h"
#include "Engine/Object/ZzzOpenData.h"
#include "Scenes/TitleSceneUI.h"
#include "UI/Core/WindowSystem.h"

// External declarations
extern EGameScene SceneFlag;
extern CErrorReport g_ErrorReport;

/**
 * @brief Webzen title/intro scene - displays loading screen and transitions to login.
 * @param hDC Device context for rendering
 */
void WebzenScene(HDC hDC)
{
    if (!OpenFont())
    {
        return;
    }
    ClearInput();

    TitleSceneUI::CreateSceneUI();

    FogEnable = false;

    ::EnableAlphaTest();
    {
        // The game draws no cursor while it loads.
        const Core::Platform::ScopedSystemCursor cursor;
        OpenBasicData(hDC);
        g_pNewUISystem->LoadMainSceneInterface();
        TitleSceneUI::RenderSceneUI(hDC, 11, 11);
    }

    TitleSceneUI::ReleaseSceneUI();

    g_ErrorReport.Write(L"> Loading ok.\r\n");

    SceneFlag = LOG_IN_SCENE;
}

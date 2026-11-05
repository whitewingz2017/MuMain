//*****************************************************************************
// File: TitleSceneUI.h
//*****************************************************************************
#pragma once

// Startup loading screen; OpenBasicData supplies its eleven real loading stages.
namespace TitleSceneUI
{
    void CreateSceneUI();
    void ReleaseSceneUI();
    void RenderSceneUI(HDC hDC, DWORD dwNow, DWORD dwTotal);
}

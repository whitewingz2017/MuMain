#include "stdafx.h"
#include "TitleSceneUI.h"
#include "Scenes/TitleLoadingRules.h"
#include "Render/Renderer/MuRenderer.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "UI/RmlBridge/RmlThemedView.h"
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Elements/ElementProgress.h>
#include <SDL3/SDL.h>

#ifdef _EDITOR
#include "../MuEditor/Core/MuEditorCore.h"
#endif

namespace
{
Rml::ElementProgress* gauge = nullptr;
Rml::Element* head = nullptr;
Rml::Element* percentage = nullptr;
int shownPercent = -1, shownFrame = -1;
UI::RmlBridge::ThemedView<> view{{{"Data/Interface/RmlUi/title_scene.rml"}},
    {.beforeUnload = [] { gauge = nullptr; head = percentage = nullptr; }}};

void UpdateProgress(DWORD now, DWORD total)
{
    if (!gauge) return;
    const float fraction = TitleSceneUI::ProgressFraction(now, total);
    gauge->SetMax(1.f);
    gauge->SetValue(fraction);
    if (head) head->SetProperty("left", std::to_string(fraction * 100.f) + "%");
    const int frame = TitleSceneUI::LoadingFrame(SDL_GetTicks());
    if (head && frame != shownFrame)
    {
        head->SetClassNames("head-frame-" + std::to_string(frame));
        shownFrame = frame;
    }
    const int percent = static_cast<int>(fraction * 100.f);
    if (!percentage || percent == shownPercent) return;
    Rml::String digits;
    for (const char digit : std::to_string(percent))
        digits += "<span class=\"digit digit-" + Rml::String(1, digit) + "\"></span>";
    percentage->SetInnerRML(digits + "<span class=\"percent-sign\"></span>");
    shownPercent = percent;
}
}

void TitleSceneUI::CreateSceneUI()
{
    ReleaseSceneUI();
    if (!view.Ensure()) return;
    auto* document = view.Document();
    gauge = rmlui_dynamic_cast<Rml::ElementProgress*>(document->GetElementById("gauge_fill"));
    head = document->GetElementById("gauge_head");
    percentage = document->GetElementById("loading_percent");
    shownPercent = shownFrame = -1;
    UpdateProgress(0, 1);
    document->Show();
}

void TitleSceneUI::ReleaseSceneUI() { view.Release(); }

void TitleSceneUI::RenderSceneUI(HDC hDC, DWORD dwNow, DWORD dwTotal)
{
    // Loading callbacks reuse an active frame so no empty frame is presented between steps.
    const bool wasFrameActive = mu::GetRenderer().IsFrameActive();
    if (!wasFrameActive) mu::GetRenderer().BeginFrame();
    ::BeginOpengl();
    mu::GetRenderer().ClearScreen();
    ::BeginBitmap();
    UpdateProgress(dwNow, dwTotal);
    ::EndBitmap();
    ::EndOpengl();
#ifdef _EDITOR
    g_MuEditorCore.Render();
#endif
    mu::GetRenderer().EndFrame();
    if (wasFrameActive) mu::GetRenderer().BeginFrame();
}

#include "stdafx.h"
#include "UI/HUD/Notices.h"
#include "UI/HUD/NoticeHistory.h"
#include "UI/HUD/NoticesRmlModel.h"
#include "UI/RmlBridge/RmlDocumentVisibility.h"
#include "UI/RmlBridge/RmlSyncField.h"
#include "UI/RmlBridge/RmlThemedView.h"
#include "UI/Core/WindowSystem.h"
#include "Core/Utilities/StringUtils.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "Render/Text/CUIRenderText.h"
#include "App/Platform/Windows/Winmain.h"
#include "Scenes/SceneCore.h"

#include <RmlUi/Core/Context.h>
#include <chrono>

namespace UI::Notices
{
namespace
{
constexpr const char* Stages[] = {"stage-in", "stage-2", "stage-3", "stage-4", "stage-5"};
NoticeHistory s_History;
bool s_RenderedThisFrame = false;
bool s_Dirty = true;
bool s_ForceRefresh = true;
std::uint64_t s_SyncedRevision = 0;
auto s_LastMove = std::chrono::steady_clock::now();

void BindModel(Rml::DataModelConstructor& constructor, NoticesRmlModel& model)
{
    auto line = constructor.RegisterStruct<NoticeLineEntry>();
    line.RegisterMember("text", &NoticeLineEntry::text);
    line.RegisterMember("kind", &NoticeLineEntry::kind);
    line.RegisterMember("stage", &NoticeLineEntry::stage);
    constructor.RegisterArray<std::vector<NoticeLineEntry>>();
    constructor.Bind("lines", &model.lines);
    constructor.Bind("outgoing_text", &model.outgoingText);
    constructor.Bind("outgoing_kind", &model.outgoingKind);
    constructor.Bind("outgoing_stage", &model.outgoingStage);
}

UI::RmlBridge::ThemedView<NoticesRmlModel> s_View{
    "notices", BindModel, {{"Data/Interface/RmlUi/notices.rml"}},
    {.afterBuild = [] { s_ForceRefresh = true; }}};

std::vector<NoticeLineEntry> BuildLines(bool preparing)
{
    std::vector<NoticeLineEntry> lines(NoticeHistory::Capacity);
    for (std::size_t index = 0; index < lines.size(); ++index)
    {
        const auto& entry = s_History.Entries()[index];
        if (entry.text.empty())
            continue;
        auto& line = lines[index];
        line.text = StringUtils::WideToNarrow(entry.text.c_str());
        line.kind = entry.guild ? "tone-guild" : "";
        line.stage = preparing ? (index == 0 ? "prep" : Rml::String(Stages[index - 1]) + " snap") : Stages[index];
    }
    return lines;
}

void SyncModel(bool preparing)
{
    auto& binder = s_View.Binder();
    const auto& outgoing = s_History.Outgoing();
    Rml::String stage = outgoing.text.empty() ? "empty" : (preparing ? "stage-5 snap" : "stage-out");
    SyncField(binder, &NoticesRmlModel::lines, "lines", BuildLines(preparing));
    SyncField(binder, &NoticesRmlModel::outgoingText, "outgoing_text", StringUtils::WideToNarrow(outgoing.text.c_str()));
    SyncField(binder, &NoticesRmlModel::outgoingKind, "outgoing_kind", Rml::String(outgoing.guild ? "tone-guild" : ""));
    SyncField(binder, &NoticesRmlModel::outgoingStage, "outgoing_stage", std::move(stage));
}

void SyncView()
{
    auto* document = s_View.Document();
    if (!document)
        return;
    UI::RmlBridge::SyncDocumentVisibility(document,
        s_History.HasMessages() || !s_History.Outgoing().text.empty());
    if (!s_Dirty && !s_ForceRefresh)
        return;
    if (s_ForceRefresh || s_SyncedRevision != s_History.Revision())
    {
        // Commit the old stages before transitioning the same stable rows to their new stages.
        SyncModel(true);
        document->GetContext()->Update();
        s_SyncedRevision = s_History.Revision();
    }
    SyncModel(false);
    s_Dirty = false;
    s_ForceRefresh = false;
}

void RenderNative()
{
    constexpr int CentreX = 320;
    constexpr int Top = 300;
    constexpr int RowHeight = 13;
    EnableAlphaTest();
    g_pRenderText->SetFont(g_hFontBold);
    for (std::size_t index = 0; index < NoticeHistory::Capacity; ++index)
    {
        const auto& entry = s_History.Entries()[index];
        if (entry.text.empty())
            continue;
        if (entry.guild)
            g_pRenderText->SetTextColor(100, 255, 200, 255);
        else
            g_pRenderText->SetTextColor(255, 200, 80, 255);
        g_pRenderText->SetBgColor(0, 0, 0, 128);
        g_pRenderText->RenderText(CentreX, Top + static_cast<int>(index) * RowHeight,
                                 entry.text.c_str(), 0, 0, RT3_WRITE_CENTER);
    }
}
} // namespace

void Clear()
{
    s_History.Clear();
    s_LastMove = std::chrono::steady_clock::now();
    s_Dirty = true;
}

void Create(const wchar_t* text, int color)
{
    // Announcements (event openings and the like) belong to the world, not the login or
    // character screens; ones that arrive there are dropped rather than replayed late.
    if (!text || !*text || SceneFlag != MAIN_SCENE)
        return;
    s_History.Push(text, color != 0);
    s_LastMove = std::chrono::steady_clock::now();
    s_Dirty = true;
}

void Move()
{
    if (!s_RenderedThisFrame)
        UI::RmlBridge::SyncDocumentVisibility(s_View.Document(), false);
    s_RenderedThisFrame = false;
    const auto now = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(now - s_LastMove).count();
    s_LastMove = now;
    s_Dirty = s_History.Advance(elapsed) || s_Dirty;
}

void Render()
{
    if (SceneFlag != MAIN_SCENE)
    {
        UI::RmlBridge::SyncDocumentVisibility(s_View.Document(), false);
        return;
    }
    s_RenderedThisFrame = true;
#ifdef KJH_ADD_INGAMESHOP_UI_SYSTEM
    if (g_pNewUISystem->IsVisible(mu::ui::window::INTERFACE_INGAMESHOP))
    {
        UI::RmlBridge::SyncDocumentVisibility(s_View.Document(), false);
        return;
    }
#endif
    s_View.Ensure();
    if (s_View.Document())
        SyncView();
    else
        RenderNative();
}
} // namespace UI::Notices

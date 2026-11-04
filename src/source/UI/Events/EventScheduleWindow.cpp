#include "stdafx.h"
#include "UI/Events/EventScheduleWindow.h"
#include "GameLogic/Events/EventSchedule.h"
#include "GameLogic/Events/EventScheduleSnapshot.h"
#include "Core/Utilities/StringUtils.h"
#include "UI/Dialogs/GenericMenuDialog.h"
#include <algorithm>
#include <chrono>

namespace UI::Events
{
void ShowSchedule(const GameLogic::Events::Schedule::Snapshot& snapshot)
{
    using namespace mu::ui::window;
    if (!g_pGenericMenuDialog) return;
    GenericMenuConfig config;
    config.title = L"Events Timer";
    config.kind = "event-timers";
    config.lines.push_back({L"Current server: active events and next scheduled starts. Refresh to check changes."});
    if (snapshot.rows.empty()) config.lines.push_back({L"No active or scheduled events on this server."});
    const auto sampled = std::chrono::steady_clock::now();
    for (const auto& row : snapshot.rows)
    {
        GenericMenuConfig::MenuButton button;
        if (row.name == "ChaosCastleStartPlugIn"){
            button.label = StringUtils::NarrowToWide("Chaos Castle");
        }else if(row.name == "BloodCastleStartPlugIn"){
            button.label = StringUtils::NarrowToWide("Blood Castle");
        }else if(row.name == "KanturuStartPlugIn"){
            button.label = StringUtils::NarrowToWide("Maya Ruins");
        }else if(row.name == "DevilSquareStartPlugIn"){
            button.label = StringUtils::NarrowToWide("Devil Square");
        }else{
            button.label = StringUtils::NarrowToWide(row.name);
        }
        
        button.enabled = false;
        button.liveDetail = [row, sampled]
        {
            const int elapsed = static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - sampled).count());
            const int seconds = std::max(0, row.seconds - elapsed);
            wchar_t clock[32]{};
            swprintf(clock, 32, L"%02d:%02d:%02d", seconds / 3600, seconds / 60 % 60, seconds % 60);
            return std::wstring(row.active ? L"Ends in " : L"Starts in ") + clock;
        };
        config.buttons.push_back(std::move(button));
    }
    GenericMenuConfig::MenuButton refresh;
    refresh.label = L"Refresh"; refresh.compact = true; refresh.onClick = GameLogic::Events::Schedule::Open;
    config.buttons.push_back(std::move(refresh));
    GenericMenuConfig::MenuButton close;
    close.label = L"Close"; close.compact = true; close.dismiss = true;
    config.buttons.push_back(std::move(close));
    g_pGenericMenuDialog->Show(std::move(config));
}
}

#include "stdafx.h"
#include "UI/Events/EventProgressWindow.h"
#include "GameLogic/Events/EventProgress.h"
#include "UI/Core/WindowManager.h"
#include "UI/RmlBridge/RmlDocumentVisibility.h"
#include "World/MapInfra/MapManager.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <string>

namespace mu::ui::window
{
namespace
{
using GameLogic::Events::ObjectiveState;
using Objective = CEventProgressWindow::Objective;
using Model = CEventProgressWindow::Model;

Rml::String Clock(int seconds)
{
    seconds = std::max(0, seconds);
    const int minutes = seconds / 60, remainder = seconds % 60;
    return (minutes < 10 ? "0" : "") + std::to_string(minutes) + ":" + (remainder < 10 ? "0" : "") + std::to_string(remainder);
}

const char* StateName(ObjectiveState state)
{
    return state == ObjectiveState::Active ? "active" : state == ObjectiveState::Complete ? "complete" : "pending";
}

Rml::String Count(const char* label, int done, int goal)
{
    return goal > 0 ? Rml::String(label) + " ( " + std::to_string(done) + " / " + std::to_string(goal) + " )" : Rml::String(label);
}

void BloodCastle(Model& m)
{
    const auto& state = GameLogic::Events::BloodCastleProgress();
    const int level = gMapManager.WorldActive == WD_52BLOODCASTLE_MASTER_LEVEL ? 8 : gMapManager.WorldActive - WD_11BLOODCASTLE1 + 1;
    m.name = "Blood Castle " + std::to_string(level);
    m.time = state.received ? Clock(GameLogic::Events::RemainingEventSeconds(state)) : "--:--";
    const Rml::String labels[] = {Count("Monsters", state.monsterKills, state.monsterGoal), "Destroy the gate",
                                  Count("Magic monsters", state.magicKills, state.magicGoal), "Destroy the statue",
                                  "Return the Archangel weapon"};
    m.objectives.clear();
    for (int i = 0; i < 5; ++i)
        m.objectives.push_back({labels[i], {},
                                StateName(state.received ? GameLogic::Events::BloodCastleObjective(state.stage, i) : ObjectiveState::Pending)});
    m.notice = state.ended ? (state.stage == 5 ? "Event completed" : "Event ended") : state.received ? "" : "Waiting for event status";
    m.scores = false;
    m.points.clear();
    m.rank.clear();
}

void DevilSquare(Model& m)
{
    const auto& state = GameLogic::Events::DevilSquareProgress();
    m.name = "Devil Square" + (state.received ? " " + std::to_string(state.level) : Rml::String{});
    const int remaining = GameLogic::Events::RemainingEventSeconds(state);
    m.time = state.received ? Clock(remaining) : "--:--";
    const int elapsed = state.seconds - remaining;
    const char* names[] = {"First wave", "Second wave", "Third wave", "The bosses rise"};
    m.objectives.clear();
    if (!state.received)
    {
        for (const char* name : names) m.objectives.push_back({name, {}, "pending"});
    }
    else
    {
        for (int i = 0; i < state.waveCount; ++i)
        {
            const auto& wave = state.waves[i];
            const Rml::String label = i < 4 ? Rml::String(names[i]) : "Wave " + std::to_string(wave.number);
            const Rml::String until = wave.state == ObjectiveState::Pending ? "in " + Clock(wave.startsIn - elapsed) : Rml::String{};
            m.objectives.push_back({label, until, StateName(wave.state)});
        }
    }
    m.notice = state.ended ? "Event ended" : state.received ? "" : "Waiting for server event data";
    m.scores = true;
    m.points = state.points >= 0 ? std::to_string(state.points) : "--";
    m.rank = state.rank > 0 ? std::to_string(state.rank) + " / " + std::to_string(state.players) : "--";
}
}

CEventProgressWindow::~CEventProgressWindow()
{
    m_view.Release();
    if (m_manager) m_manager->RemoveUIObj(this);
}

void CEventProgressWindow::Create(CManager* manager)
{
    m_manager = manager;
    manager->AddUIObj(INTERFACE_EVENT_PROGRESS, this);
    Show(false);
}

void CEventProgressWindow::Bind(Rml::DataModelConstructor& c, Model& model)
{
    auto objective = c.RegisterStruct<Objective>();
    objective.RegisterMember("label", &Objective::label);
    objective.RegisterMember("countdown", &Objective::countdown);
    objective.RegisterMember("state", &Objective::state);
    c.RegisterArray<std::vector<Objective>>();
    c.Bind("name", &model.name);
    c.Bind("time", &model.time);
    c.Bind("notice", &model.notice);
    c.Bind("scores", &model.scores);
    c.Bind("points", &model.points);
    c.Bind("rank", &model.rank);
    c.Bind("objectives", &model.objectives);
}

bool CEventProgressWindow::Update()
{
    const bool blood = gMapManager.InBloodCastle(), devil = !blood && gMapManager.InDevilSquare();
    const bool active = blood || devil;
    if (!active && !m_view.Document()) return true;
    if (!m_view.Ensure()) return true;
    UI::RmlBridge::SyncDocumentVisibilityBehind(m_view.Document(), active);
    if (!active) return true;

    // The clocks tick in seconds; four refreshes a second keep them on time without per-frame churn.
    const Uint64 now = SDL_GetTicks();
    if (m_lastSync != 0 && now - m_lastSync < 250) return true;
    m_lastSync = now;

    Model next = m_view.GetModel();
    if (blood) BloodCastle(next);
    else DevilSquare(next);
    auto& model = m_view.GetModel();
    const auto sync = [&](auto Model::* field, const char* name)
    {
        if (model.*field == next.*field) return;
        model.*field = std::move(next.*field);
        m_view.MarkDirty(name);
    };
    sync(&Model::name, "name");
    sync(&Model::time, "time");
    sync(&Model::notice, "notice");
    sync(&Model::scores, "scores");
    sync(&Model::points, "points");
    sync(&Model::rank, "rank");
    sync(&Model::objectives, "objectives");
    return true;
}
}

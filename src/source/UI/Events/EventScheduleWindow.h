#pragma once
namespace GameLogic::Events::Schedule { struct Snapshot; }
namespace UI::Events { void ShowSchedule(const GameLogic::Events::Schedule::Snapshot& snapshot); }

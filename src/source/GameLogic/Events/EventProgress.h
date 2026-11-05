#pragma once
#include <array>
#include <chrono>
#include <cstdint>
#include <span>

// Live Blood Castle / Devil Square objective progress for the event progress panel
// (UI/Events/EventProgressWindow). Blood Castle is derived from the match state packet;
// Devil Square comes from the server's C2 FA 30 progress packet.
namespace GameLogic::Events
{
enum class ObjectiveState { Pending, Active, Complete };
struct EventProgressState
{
    bool received = false;
    bool ended = false;
    int level = 0, seconds = 0, stage = 0;
    int monsterKills = 0, monsterGoal = 0, magicKills = 0, magicGoal = 0;
    int points = -1, rank = 0, players = 0, waveCount = 0;
    struct Wave { int number = 0, startsIn = 0, endsIn = 0; ObjectiveState state = ObjectiveState::Pending; };
    std::array<Wave, 8> waves{};
    std::chrono::steady_clock::time_point sampled{};
};
const EventProgressState& BloodCastleProgress();
const EventProgressState& DevilSquareProgress();
void ResetEventProgress();
void UpdateBloodCastleProgress(int playState, int seconds, int killed, int goal, int itemType);
void CompleteBloodCastleProgress(bool success);
bool ReceiveDevilSquareProgress(std::span<const std::uint8_t> packet);
int RemainingEventSeconds(const EventProgressState& state);
ObjectiveState BloodCastleObjective(int stage, int objective);
constexpr bool IsArchangelCarrierType(int type) { return type >= 1 && type <= 3; }
}

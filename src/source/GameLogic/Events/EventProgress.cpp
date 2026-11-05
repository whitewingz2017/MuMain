#include "stdafx.h"
#include "GameLogic/Events/EventProgress.h"
#include <algorithm>

namespace GameLogic::Events
{
namespace
{
EventProgressState blood, devil;
int Word(std::span<const std::uint8_t> data, std::size_t offset) { return data[offset] | (data[offset + 1] << 8); }
}
const EventProgressState& BloodCastleProgress() { return blood; }
const EventProgressState& DevilSquareProgress() { return devil; }
void ResetEventProgress() { blood = {}; devil = {}; }
ObjectiveState BloodCastleObjective(int stage, int objective)
{
    return objective < stage ? ObjectiveState::Complete : objective == stage ? ObjectiveState::Active : ObjectiveState::Pending;
}
void UpdateBloodCastleProgress(int playState, int seconds, int killed, int goal, int itemType)
{
    if (playState == 3) return; // Bridge animation command carries no timer/count snapshot.
    if (playState != 0 && playState != 1 && playState != 2 && playState != 4) return;
    if (playState == 0) blood = {};
    blood.received = true;
    blood.seconds = std::max(0, seconds);
    blood.sampled = std::chrono::steady_clock::now();
    blood.ended = playState == 2;
    if (blood.ended) return;
    int stage = blood.stage;
    if (playState <= 1)
    {
        blood.monsterKills = std::max(0, killed); blood.monsterGoal = std::max(0, goal);
        stage = goal > 0 && killed >= goal ? 1 : 0;
    }
    if (playState == 4)
    {
        if (goal > 0) { blood.magicKills = std::max(0, killed); blood.magicGoal = goal; }
        blood.monsterKills = blood.monsterGoal;
        if (goal <= 0) blood.magicKills = blood.magicGoal;
        stage = goal > 0 && killed < goal ? 2 : 3;
    }
    if (IsArchangelCarrierType(itemType)) stage = 4;
    blood.stage = std::max(blood.stage, stage);
}
void CompleteBloodCastleProgress(bool success) { blood.seconds = 0; blood.ended = true; if (success) blood.stage = 5; }

bool ReceiveDevilSquareProgress(std::span<const std::uint8_t> packet)
{
    constexpr std::size_t Header = 5, Fixed = 13, WaveBytes = 6;
    if (packet.size() < Header + Fixed || packet[0] != 0xC2 || packet[3] != 0xFA || packet[4] != 0x30 ||
        ((packet[1] << 8) | packet[2]) != packet.size()) return false;
    const auto data = packet.subspan(Header);
    const int count = data[12];
    if (data[0] != 1 || data[1] != 1 || data[2] < 1 || data[2] > 7 || data[3] > 1 || count > 8 ||
        data.size() != Fixed + count * WaveBytes || data[10] > data[11]) return false;
    EventProgressState next;
    next.received = true; next.ended = data[3] != 0; next.level = data[2]; next.seconds = Word(data, 4);
    const std::uint32_t points = data[6] | (std::uint32_t(data[7]) << 8) | (std::uint32_t(data[8]) << 16) | (std::uint32_t(data[9]) << 24);
    if (points > 0x7FFFFFFFu) return false;
    next.points = static_cast<int>(points); next.rank = data[10]; next.players = data[11]; next.waveCount = count;
    for (int i = 0; i < count; ++i)
    {
        const auto offset = Fixed + i * WaveBytes;
        if (data[offset + 1] > 2) return false;
        next.waves[i] = {data[offset], Word(data, offset + 2), Word(data, offset + 4), static_cast<ObjectiveState>(data[offset + 1])};
    }
    next.sampled = std::chrono::steady_clock::now();
    devil = next;
    return true;
}
int RemainingEventSeconds(const EventProgressState& state)
{
    if (!state.received || state.ended) return 0;
    const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - state.sampled).count();
    return std::max(0, state.seconds - static_cast<int>(elapsed));
}
}

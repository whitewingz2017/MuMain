#pragma once

#include <cmath>
#include <algorithm>

namespace GameLogic::Cosmetics
{
// One step of a pet or town mount walking to its follow spot. A tile is 100 units and a walking
// owner crosses one every 300 ms (about 13 units per 25 fps frame), more with speed buffs, so a
// fixed 8-unit pace fell behind. Closing a share of the gap keeps up with any owner speed and
// still slows down smoothly at the spot.
inline float GroundFollowStep(float distance, float frameFactor)
{
    constexpr float MinimumSpeedPerNormalFrame = 8.f;
    constexpr float CatchUpPerNormalFrame = 0.25f;
    const float speed = std::max(MinimumSpeedPerNormalFrame, distance * CatchUpPerNormalFrame);
    return std::min(distance, speed * frameFactor);
}

struct AnimationBlend
{
    float current;
    float prior;
};

// BMD interpolates prior -> integer(current), using the fraction of current as its blend weight.
inline AnimationBlend LoopAnimationBlend(float phase, int cycleKeys)
{
    if (cycleKeys <= 1) return {0.f, 0.f};
    const float wrapped = std::fmod(std::fmod(phase, static_cast<float>(cycleKeys)) + cycleKeys, static_cast<float>(cycleKeys));
    const int lower = static_cast<int>(wrapped);
    return {static_cast<float>((lower + 1) % cycleKeys) + wrapped - lower, static_cast<float>(lower)};
}

inline float AnimationPhase(double elapsedMs, int keys, bool lockedPositions, float speed)
{
    const int cycleKeys = keys - (lockedPositions ? 1 : 0);
    if (cycleKeys <= 1) return 0.f;
    constexpr double NormalFramesPerSecond = 25.0;
    constexpr double MillisecondsPerSecond = 1000.0;
    return static_cast<float>(std::fmod(elapsedMs * NormalFramesPerSecond * speed / MillisecondsPerSecond, cycleKeys));
}

inline float WingAnimationFrame(double elapsedMs, int keys, bool lockedPositions, bool flying)
{
    constexpr float IdleSpeed = 0.25f;
    constexpr float FlightSpeed = 1.f;
    // FinalMU advances each wing's own keys at the standard idle/flight speed.
    return AnimationPhase(elapsedMs, keys, lockedPositions, flying ? FlightSpeed : IdleSpeed);
}

inline int MountAnimationAction(int action, int actionCount)
{
    constexpr int StandAction = 0;
    constexpr int AttackAction = 3;
    constexpr int SkillAction = 4;
    if (action >= 0 && action < actionCount) return action;
    return action == SkillAction && actionCount > AttackAction ? AttackAction : StandAction;
}

inline AnimationBlend MountAnimationBlend(float frame, int sourceKeys, int targetKeys)
{
    const float phase = sourceKeys > 1 ? (frame - 1.f) / sourceKeys * targetKeys : 0.f;
    return LoopAnimationBlend(phase, targetKeys);
}

inline int FinalMuFollowerAction(bool moving, double idleMs, int actionCount)
{
    constexpr int MovingAction = 2;
    constexpr int IdleGestureAction = 3;
    constexpr double IdleCycleMs = 15000.0;
    constexpr double WalkStartMs = 4000.0, WalkEndMs = 6000.0;
    constexpr double GestureStartMs = 10000.0, GestureEndMs = 14000.0;
    const double phase = std::fmod(std::max(0.0, idleMs), IdleCycleMs);
    int action = 0;
    if (moving || phase >= GestureEndMs) action = MovingAction;
    else if (phase >= GestureStartMs) action = IdleGestureAction;
    else if (phase >= WalkStartMs && phase < WalkEndMs) action = 1;
    return action < actionCount ? action : moving && actionCount > 1 ? 1 : 0;
}

inline float OrbitFacing(float angleToOwner)
{
    constexpr float ForwardTangentDegrees = 90.f;
    constexpr float FullTurnDegrees = 360.f;
    return std::fmod(angleToOwner + ForwardTangentDegrees + FullTurnDegrees, FullTurnDegrees);
}
}

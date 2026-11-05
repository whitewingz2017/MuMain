#include "GameLogic/Cosmetics/WingAnimationClock.h"
#include "GameLogic/Cosmetics/CosmeticAnimationRules.h"

namespace GameLogic::Cosmetics
{
float WingAnimationClock::Advance(int model, double timeMs, int keys, bool lockedPositions, bool flying, bool paused)
{
    if (model != m_model || timeMs < m_lastMs)
    {
        m_model = model;
        m_lastMs = timeMs;
        m_phase = 0.f;
    }
    const double elapsedMs = timeMs - m_lastMs;
    m_lastMs = timeMs;
    const int cycleKeys = keys - (lockedPositions ? 1 : 0);
    if (cycleKeys <= 1) return m_phase = 0.f;
    if (!paused)
        m_phase = std::fmod(m_phase + WingAnimationFrame(elapsedMs, keys, lockedPositions, flying), cycleKeys);
    return m_phase;
}

void WingAnimationClock::Reset()
{
    *this = WingAnimationClock{};
}
}

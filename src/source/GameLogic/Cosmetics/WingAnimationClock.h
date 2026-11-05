#pragma once

namespace GameLogic::Cosmetics
{
// A skin's pose survives the temporary equipment override and changes speed without changing phase.
class WingAnimationClock
{
public:
    float Advance(int model, double timeMs, int keys, bool lockedPositions, bool flying, bool paused);
    void Reset();

private:
    int m_model = -1;
    double m_lastMs = -1.0;
    float m_phase = 0.f;
};
}

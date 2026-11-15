#include "stdafx.h"
#include "Core/Time/FramePacing.h"

#include <algorithm>
#include <cmath>

namespace Core::Time::FramePacing
{
namespace
{
// Elapsed time a hitch may leave for later frames to advance. Longer stalls (loading, a
// dragged window) are dropped, so the game never fast-forwards through them.
constexpr double MaxCatchUpMs = 200.0;

// How far a frame may stray from whole intervals and still count as on schedule: a quarter
// interval covers timer and wake-up jitter, not a frame that missed its deadline.
constexpr double OnScheduleToleranceIntervals = 0.25;

// The most a snapped step moves toward the elapsed time per frame. It covers a display whose real
// refresh differs slightly from the reported one (59.94 Hz reported as 60), and stays well below
// frame-to-frame jitter, which cancels out over the next frames on its own.
constexpr double MaxCorrectionIntervals = 0.005;

// Weight of the newest frame in the average grid offset, and how far that average may sit from
// whole intervals before frames no longer count as taking whole intervals: a variable refresh
// display, or a refresh rate other than the reported one.
constexpr double GridOffsetAverageWeight = 0.1;
constexpr double MaxAverageGridOffset = 0.05;

// A step is never zero: per-frame code divides by the frame factor.
constexpr double MinStepMs = 0.001;

void UpdateAverageGridOffset(SimulationClock& clock, double elapsedMs, double frameIntervalMs)
{
    const double intervals = elapsedMs / frameIntervalMs;
    const double offset = intervals - std::round(intervals);
    clock.averageGridOffset += (offset - clock.averageGridOffset) * GridOffsetAverageWeight;
}

bool FramesTakeWholeIntervals(const SimulationClock& clock)
{
    return std::abs(clock.averageGridOffset) <= MaxAverageGridOffset;
}

double SnapToFrameIntervals(double pendingMs, double frameIntervalMs)
{
    const double wholeIntervals = std::round(pendingMs / frameIntervalMs);
    if (wholeIntervals < 1.0)
    {
        return pendingMs;
    }

    const double snappedMs = wholeIntervals * frameIntervalMs;
    const double driftMs = pendingMs - snappedMs;
    if (std::abs(driftMs) > frameIntervalMs * OnScheduleToleranceIntervals)
    {
        return pendingMs;
    }

    const double maxCorrectionMs = frameIntervalMs * MaxCorrectionIntervals;
    return snappedMs + std::clamp(driftMs, -maxCorrectionMs, maxCorrectionMs);
}
} // namespace

double NextFrameDeadline(double frameDeadlineMs, double nowMs, double frameIntervalMs)
{
    const double nextDeadlineMs = frameDeadlineMs + frameIntervalMs;
    if (nextDeadlineMs < nowMs)
    {
        return nowMs + frameIntervalMs;
    }
    return nextDeadlineMs;
}

double TakeSimulationStepMs(SimulationClock& clock, double elapsedMs, double frameIntervalMs, double maxStepMs)
{
    clock.pendingMs = std::min(clock.pendingMs + elapsedMs, MaxCatchUpMs);
    double stepMs = clock.pendingMs;
    if (frameIntervalMs > 0.0)
    {
        UpdateAverageGridOffset(clock, elapsedMs, frameIntervalMs);
        if (FramesTakeWholeIntervals(clock))
        {
            stepMs = SnapToFrameIntervals(clock.pendingMs, frameIntervalMs);
        }
    }

    stepMs = std::clamp(stepMs, MinStepMs, maxStepMs);
    clock.pendingMs -= stepMs;
    return stepMs;
}
} // namespace Core::Time::FramePacing

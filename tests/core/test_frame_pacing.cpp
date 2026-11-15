#include "doctest.h"

#include "Core/Time/FramePacing.h"
#include "Scenes/SceneManager.h"

#include <algorithm>
#include <array>
#include <cmath>

using Core::Time::FramePacing::NextFrameDeadline;
using Core::Time::FramePacing::SimulationClock;
using Core::Time::FramePacing::TakeSimulationStepMs;

namespace
{
constexpr double IntervalMs = 1000.0 / 144.0;
constexpr double ReferenceFrameMs = 40.0;
// How far a snapped step may move toward the elapsed time in one frame.
constexpr double MaxCorrection = 0.005;
// Timer and wake-up jitter: each frame's lateness is made up by the next, as on a fixed schedule.
constexpr std::array<double, 4> JitterMs{0.3, -0.3, -0.2, 0.2};

bool IsWholeIntervals(double stepMs, double intervalMs)
{
    const double intervals = std::round(stepMs / intervalMs);
    return intervals >= 1.0 && std::abs(stepMs - intervals * intervalMs) <= intervalMs * MaxCorrection + 1e-9;
}
} // namespace

TEST_CASE("A capped frame rate keeps a fixed schedule of deadlines [core][timing]")
{
    SUBCASE("a frame on time is due one interval after its deadline")
    {
        CHECK(NextFrameDeadline(100.0, 100.0, IntervalMs) == doctest::Approx(100.0 + IntervalMs));
    }

    SUBCASE("a late frame shortens the next interval instead of shifting the schedule")
    {
        CHECK(NextFrameDeadline(100.0, 100.0 + IntervalMs * 0.5, IntervalMs) == doctest::Approx(100.0 + IntervalMs));
    }

    SUBCASE("more than an interval behind, the schedule restarts from now")
    {
        const double now = 100.0 + IntervalMs * 3.0;
        CHECK(NextFrameDeadline(100.0, now, IntervalMs) == doctest::Approx(now + IntervalMs));
    }
}

TEST_CASE("A frame timing state renders on its deadlines [core][timing]")
{
    FrameTimingState timing;
    timing.SetTargetFps(100.0);
    timing.UpdateCurrentTime(1000.0);
    REQUIRE(timing.ShouldRenderNextFrame());
    timing.MarkFrameRendered();

    timing.UpdateCurrentTime(1009.0);
    CHECK_FALSE(timing.ShouldRenderNextFrame());
    CHECK(timing.GetTimeUntilNextFrame() == doctest::Approx(1.0));

    // Started 2 ms late: the following frame is still due on the original schedule.
    timing.UpdateCurrentTime(1012.0);
    REQUIRE(timing.ShouldRenderNextFrame());
    timing.MarkFrameRendered();
    timing.UpdateCurrentTime(1019.9);
    CHECK_FALSE(timing.ShouldRenderNextFrame());
    timing.UpdateCurrentTime(1020.0);
    CHECK(timing.ShouldRenderNextFrame());

    timing.SetTargetFps(-1.0);
    CHECK(timing.ShouldRenderNextFrame());
    CHECK(timing.GetTimeUntilNextFrame() == 0.0);
}

TEST_CASE("Frames on schedule advance one interval each, whatever their jitter [core][timing]")
{
    SimulationClock clock;
    double simulatedMs = 0.0;
    double elapsedTotalMs = 0.0;
    for (int frame = 0; frame < 1000; ++frame)
    {
        const double elapsedMs = IntervalMs + JitterMs[frame % JitterMs.size()];
        const double stepMs = TakeSimulationStepMs(clock, elapsedMs, IntervalMs, ReferenceFrameMs);
        CHECK(stepMs == doctest::Approx(IntervalMs).epsilon(MaxCorrection));
        simulatedMs += stepMs;
        elapsedTotalMs += elapsedMs;
    }
    CHECK(std::abs(elapsedTotalMs - simulatedMs) <= IntervalMs);
}

TEST_CASE("VSync steps follow a display whose real refresh differs from the reported one [core][timing]")
{
    const double reportedIntervalMs = 1000.0 / 60.0;
    const double realIntervalMs = 1000.0 / 59.94;
    SimulationClock clock;
    double simulatedMs = 0.0;
    double elapsedTotalMs = 0.0;
    for (int frame = 0; frame < 6000; ++frame)
    {
        const double stepMs = TakeSimulationStepMs(clock, realIntervalMs, reportedIntervalMs, ReferenceFrameMs);
        CHECK(stepMs == doctest::Approx(reportedIntervalMs).epsilon(MaxCorrection));
        simulatedMs += stepMs;
        elapsedTotalMs += realIntervalMs;
    }
    CHECK(std::abs(elapsedTotalMs - simulatedMs) <= reportedIntervalMs * 0.25);
}

TEST_CASE("A frame shown for several refresh intervals advances all of them [core][timing]")
{
    const double refreshMs = 1000.0 / 120.0;
    SimulationClock clock;
    for (int frame = 0; frame < 400; ++frame)
    {
        const double intervals = (frame % 3 == 0) ? 2.0 : 1.0;
        const double elapsedMs = refreshMs * intervals + JitterMs[frame % JitterMs.size()] * 0.5;
        const double stepMs = TakeSimulationStepMs(clock, elapsedMs, refreshMs, ReferenceFrameMs);
        CHECK(IsWholeIntervals(stepMs, refreshMs));
    }
}

TEST_CASE("Frames that do not take whole intervals advance their real time [core][timing]")
{
    // A variable refresh display shows each frame when it is ready, here at 1.11 refresh intervals.
    SimulationClock clock;
    const double elapsedMs = IntervalMs * 1.11;
    for (int frame = 0; frame < 30; ++frame)
    {
        TakeSimulationStepMs(clock, elapsedMs, IntervalMs, ReferenceFrameMs);
    }
    for (int frame = 0; frame < 100; ++frame)
    {
        CHECK(TakeSimulationStepMs(clock, elapsedMs, IntervalMs, ReferenceFrameMs) == doctest::Approx(elapsedMs));
    }
}

TEST_CASE("Uncapped frames advance their elapsed time [core][timing]")
{
    SimulationClock clock;
    for (const double elapsedMs : {3.1, 5.7, 4.2, 9.9})
    {
        CHECK(TakeSimulationStepMs(clock, elapsedMs, 0.0, ReferenceFrameMs) == doctest::Approx(elapsedMs));
    }
    CHECK(clock.pendingMs == doctest::Approx(0.0));
}

TEST_CASE("A frame that missed its deadline advances its real time [core][timing]")
{
    SimulationClock clock;
    const double elapsedMs = IntervalMs * 1.6;
    CHECK(TakeSimulationStepMs(clock, elapsedMs, IntervalMs, ReferenceFrameMs) == doctest::Approx(elapsedMs));
}

TEST_CASE("A hitch is advanced by the following frames, one reference frame at most each [core][timing]")
{
    for (const double intervalMs : {0.0, IntervalMs})
    {
        CAPTURE(intervalMs);
        SimulationClock clock;
        const double hitchMs = 100.0;
        const double normalMs = intervalMs > 0.0 ? intervalMs : 5.0;
        double simulatedMs = TakeSimulationStepMs(clock, hitchMs, intervalMs, ReferenceFrameMs);
        CHECK(simulatedMs == doctest::Approx(ReferenceFrameMs));

        double elapsedTotalMs = hitchMs;
        for (int frame = 0; frame < 20; ++frame)
        {
            const double stepMs = TakeSimulationStepMs(clock, normalMs, intervalMs, ReferenceFrameMs);
            CHECK(stepMs <= ReferenceFrameMs);
            simulatedMs += stepMs;
            elapsedTotalMs += normalMs;
        }
        CHECK(simulatedMs == doctest::Approx(elapsedTotalMs).epsilon(0.01));
    }
}

TEST_CASE("A long stall is dropped instead of fast-forwarded [core][timing]")
{
    SimulationClock clock;
    double simulatedMs = TakeSimulationStepMs(clock, 5000.0, 0.0, ReferenceFrameMs);
    for (int frame = 0; frame < 100; ++frame)
    {
        simulatedMs += TakeSimulationStepMs(clock, 5.0, 0.0, ReferenceFrameMs);
    }
    const double maxCatchUpMs = 200.0;
    CHECK(simulatedMs <= maxCatchUpMs + 100 * 5.0 + 0.01);
}

TEST_CASE("A step is never zero [core][timing]")
{
    SimulationClock clock;
    clock.pendingMs = -IntervalMs * 0.25;
    CHECK(TakeSimulationStepMs(clock, 0.0, IntervalMs, ReferenceFrameMs) > 0.0);
}

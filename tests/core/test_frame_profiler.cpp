#include "doctest.h"

#include "Core/Utilities/FrameProfiler.h"

TEST_CASE("renderer counters follow nested frame passes [core][profiling][frame_profiler]")
{
    using Counter = FrameProfiler::Counter;
    using Pass = FrameProfiler::Pass;

    FrameProfiler::ResetCounters();
    FrameProfiler::g_CountersEnabled = true;

    {
        FRAME_PROFILE(Terrain);
        FrameProfiler::Count(Counter::DrawCalls);

        {
            FRAME_PROFILE(Objects);
            FrameProfiler::Count(Counter::MergedDraws, 2);
            FrameProfiler::Count(Counter::Merged2DDraws);
            FrameProfiler::Count(Counter::GlyphUploads, 3);
            FrameProfiler::Count(Counter::GpuSkinningSubmissions, 4);
            FrameProfiler::Count(Counter::CpuSkinningIneligible, 2);
            FrameProfiler::Count(Counter::GpuSkinningFailures);
        }

        CHECK(FrameProfiler::CurrentPass() == Pass::Terrain);
    }

    CHECK(FrameProfiler::CurrentPass() == Pass::Other);
    CHECK(FrameProfiler::CounterValue(Pass::Terrain, Counter::DrawCalls) == 1);
    CHECK(FrameProfiler::CounterValue(Pass::Objects, Counter::MergedDraws) == 2);
    CHECK(FrameProfiler::CounterValue(Pass::Objects, Counter::Merged2DDraws) == 1);
    CHECK(FrameProfiler::CounterValue(Pass::Objects, Counter::GlyphUploads) == 3);
    CHECK(FrameProfiler::CounterValue(Pass::Objects, Counter::GpuSkinningSubmissions) == 4);
    CHECK(FrameProfiler::CounterValue(Pass::Objects, Counter::CpuSkinningIneligible) == 2);
    CHECK(FrameProfiler::CounterValue(Pass::Objects, Counter::GpuSkinningFailures) == 1);
    CHECK(FrameProfiler::CounterValue(Counter::MergedDraws) == 2);
    CHECK(FrameProfiler::CounterValue(Counter::Merged2DDraws) == 1);
    CHECK(FrameProfiler::CounterValue(Counter::GlyphUploads) == 3);
    CHECK(FrameProfiler::CounterValue(Counter::GpuSkinningSubmissions) == 4);
    CHECK(FrameProfiler::CounterValue(Counter::CpuSkinningIneligible) == 2);
    CHECK(FrameProfiler::CounterValue(Counter::GpuSkinningFailures) == 1);

    FrameProfiler::g_CountersEnabled = false;
    FrameProfiler::ResetCounters();
}

TEST_CASE("packet child passes publish separate timings and counters [core][profiling][frame_profiler]")
{
    using Counter = FrameProfiler::Counter;
    using Pass = FrameProfiler::Pass;
    const Pass children[] = {Pass::PacketDrain, Pass::Reconnect, Pass::FrameTimers, Pass::ControlPoll};
    FrameProfiler::ResetFrame();
    FrameProfiler::ResetCounters();
    FrameProfiler::g_CountersEnabled = true;
    {
        FRAME_PROFILE(Packets);
        float childMilliseconds = 1.0f;
        for (Pass child : children)
        {
            {
                FrameProfiler::Scope scope(child);
                FrameProfiler::Count(Counter::DrawCalls);
            }
            CHECK(FrameProfiler::CurrentPass() == Pass::Packets);
            CHECK(FrameProfiler::NestedPass(child));
            FrameProfiler::AccumulatorMs(child) = childMilliseconds;
            childMilliseconds += 1.0f;
        }
    }
    constexpr float ParentMilliseconds = 20.0f;
    FrameProfiler::AccumulatorMs(Pass::Packets) = ParentMilliseconds;
    FrameProfiler::CompleteFrame();
    CHECK(FrameProfiler::CompletedMs(Pass::Packets) == ParentMilliseconds);
    float expected = 1.0f;
    for (Pass child : children)
    {
        CHECK(FrameProfiler::CompletedMs(child) == expected);
        CHECK(FrameProfiler::CompletedCounter(child, Counter::DrawCalls) == 1);
        CHECK(FrameProfiler::AccumulatorMs(child) == 0.0f);
        expected += 1.0f;
    }
    FrameProfiler::g_CountersEnabled = false;
    FrameProfiler::CompleteFrame();
}

TEST_CASE("frame profile macro supports two scopes on one source line [core][profiling][frame_profiler]")
{
    { FRAME_PROFILE(Terrain); } { FRAME_PROFILE(Objects); }
    CHECK(true);
}

TEST_CASE("completed frame includes late RmlUi work and stays stable during the next frame [core][profiling][frame_profiler]")
{
    using Counter = FrameProfiler::Counter;
    using Pass = FrameProfiler::Pass;
    FrameProfiler::ResetFrame();
    FrameProfiler::ResetCounters();
    FrameProfiler::CompleteFrame();

    // Background context before the overlay, main context during renderer finalization.
    FrameProfiler::AccumulatorMs(Pass::RmlUiUpdate) = 2.0f;
    CHECK(FrameProfiler::CompletedMs(Pass::RmlUiUpdate) == 0.0f);
    FrameProfiler::AccumulatorMs(Pass::RmlUiUpdate) += 3.0f;
    FrameProfiler::AccumulatorMs(Pass::RmlUiChatLayout) = 0.5f;
    FrameProfiler::AccumulatorMs(Pass::Overlay) = 1.0f;
    FrameProfiler::CounterValue(Pass::RmlUiRender, Counter::DrawCalls) = 4;
    FrameProfiler::CounterValue(Counter::DrawCalls) = 4;
    FrameProfiler::CompleteFrame();

    CHECK(FrameProfiler::CompletedMs(Pass::RmlUiUpdate) == 5.0f);
    CHECK(FrameProfiler::CompletedMs(Pass::RmlUiChatLayout) == 0.5f);
    CHECK(FrameProfiler::CompletedMs(Pass::Overlay) == 1.0f);
    CHECK(FrameProfiler::CompletedCounter(Pass::RmlUiRender, Counter::DrawCalls) == 4);
    CHECK(FrameProfiler::CompletedCounter(Counter::DrawCalls) == 4);
    CHECK(FrameProfiler::AccumulatorMs(Pass::RmlUiUpdate) == 0.0f);
    CHECK(FrameProfiler::AccumulatorMs(Pass::RmlUiChatLayout) == 0.0f);
    CHECK(FrameProfiler::CounterValue(Counter::DrawCalls) == 0);
    CHECK(FrameProfiler::CounterValue(Pass::RmlUiRender, Counter::DrawCalls) == 0);

    // The next frame's early work must not contaminate the snapshot being displayed.
    FrameProfiler::AccumulatorMs(Pass::RmlUiUpdate) = 7.0f;
    FrameProfiler::CounterValue(Counter::DrawCalls) = 9;
    CHECK(FrameProfiler::CompletedMs(Pass::RmlUiUpdate) == 5.0f);
    CHECK(FrameProfiler::CompletedCounter(Counter::DrawCalls) == 4);
    FrameProfiler::CompleteFrame();
    CHECK(FrameProfiler::CompletedMs(Pass::RmlUiUpdate) == 7.0f);
    CHECK(FrameProfiler::CompletedMs(Pass::RmlUiChatLayout) == 0.0f);
    CHECK(FrameProfiler::CompletedMs(Pass::Overlay) == 0.0f);
    CHECK(FrameProfiler::CompletedCounter(Counter::DrawCalls) == 9);
    CHECK(FrameProfiler::CompletedCounter(Pass::RmlUiRender, Counter::DrawCalls) == 0);
    FrameProfiler::CompleteFrame();
}

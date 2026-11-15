#pragma once

// Frame pacing for a capped frame rate, and the game time each rendered frame advances.
namespace Core::Time::FramePacing
{
// The deadline of the frame after one that started at nowMs, keeping a capped frame rate on a
// fixed schedule: a frame that started late shortens the next interval instead of shifting every
// later frame. More than a whole interval behind, the schedule restarts from nowMs rather than
// rendering a burst of frames to catch up.
double NextFrameDeadline(double frameDeadlineMs, double nowMs, double frameIntervalMs);

// Elapsed time the game has not advanced yet, and how closely recent frames have taken whole
// display intervals. The caller keeps one between frames.
struct SimulationClock
{
    double pendingMs = 0.0;
    double averageGridOffset = 0.0; // recent frames' signed distance from whole intervals, in intervals
};

// Takes the game time this frame advances, in milliseconds.
// - frameIntervalMs is the interval frames reach the screen at: the frame cap's, or the display
//   refresh's with VSync. 0 when frames are shown as soon as they are ready.
// - While frames keep taking whole intervals, a frame on schedule advances whole intervals, so the
//   game moves by the same amount every displayed frame. Timing jitter stays pending, and a small
//   mismatch between the real and the reported refresh is corrected a little each frame.
// - Otherwise (uncapped, a variable refresh display, a missed deadline) the frame advances the
//   elapsed time itself.
// - No frame advances more than maxStepMs. What a hitch leaves over is advanced by the following
//   frames instead of being dropped, up to a bounded catch-up.
double TakeSimulationStepMs(SimulationClock& clock, double elapsedMs, double frameIntervalMs, double maxStepMs);
} // namespace Core::Time::FramePacing

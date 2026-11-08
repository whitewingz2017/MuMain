#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <optional>

namespace Render::PostProcessing
{

// The share of the previous frames in each pixel temporal anti-aliasing resolves; the rest is the
// current frame. Higher smooths more and converges slower after a change.
inline constexpr float TemporalHistoryWeight = 0.9f;

// What one frame of temporal anti-aliasing draws and resolves with.
struct TemporalFrame
{
    // The scene's offset this frame, in clip space (TemporalJitterClip).
    glm::vec2 jitterClip{0.0f};
    // From this frame's unjittered clip space to the previous frame's (CurrentToPreviousClip).
    glm::mat4 currentToPrevious{1.0f};
    // The previous frames' share of the result: 0 when there is no history to blend with.
    float historyWeight = 0.0f;
};

// The frames temporal anti-aliasing has blended so far: each frame starts from the camera and size
// of the one before it.
class TemporalHistory
{
public:
    // Starts a frame of `width` x `height` whose 3D scene is seen through `camera` (its
    // view-projection transform). The history blends in only when the previous frame was resolved
    // too, at the same size.
    [[nodiscard]] TemporalFrame BeginFrame(const glm::mat4& camera, std::uint32_t width, std::uint32_t height);

    // Forgets the history: the next frame starts from itself alone.
    void Reset();

private:
    std::optional<glm::mat4> m_previousCamera;
    std::uint32_t m_width = 0u;
    std::uint32_t m_height = 0u;
    std::uint32_t m_frameIndex = 0u;
};

} // namespace Render::PostProcessing

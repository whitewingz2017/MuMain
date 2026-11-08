#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <optional>

// The arithmetic of temporal anti-aliasing (post_temporal.frag.hlsl), kept free of the GPU so it can
// be checked on its own. Each frame draws the 3D scene shifted by a different fraction of a pixel;
// blended over several frames, every pixel averages several positions inside itself, which smooths
// its edges like extra samples would.
namespace Render::PostProcessing
{

// Frames after which the sub-pixel offsets repeat.
inline constexpr std::uint32_t TemporalJitterFrameCount = 8u;

// The offset frame `frameIndex` draws the scene at, in pixels (x right, y down), inside half a pixel
// of the pixel centre: the points of the Halton (2, 3) sequence, which cover a pixel evenly however
// few of them are taken.
[[nodiscard]] glm::vec2 TemporalJitterPixels(std::uint32_t frameIndex);

// `pixels` on a target of `width` x `height` as an offset in clip space (normalised device
// coordinates, y up). Zero for an empty target.
[[nodiscard]] glm::vec2 TemporalJitterClip(glm::vec2 pixels, std::uint32_t width, std::uint32_t height);

// True when `transform` projects with perspective (the 3D scene); false for an orthographic one
// (2D draws), whose pixels stay where they are.
[[nodiscard]] bool IsPerspective(const glm::mat4& transform);

// `transform` with its projected positions moved by `clipOffset` in clip space.
[[nodiscard]] glm::mat4 JitterTransform(const glm::mat4& transform, glm::vec2 clipOffset);

// The transform from the current frame's clip space to the previous frame's, for scenes seen through
// the cameras (view-projection transforms) `previousCamera` and `currentCamera`: where a surface
// drawn now was drawn in the previous frame. nullopt when the current camera cannot be inverted.
[[nodiscard]] std::optional<glm::mat4> CurrentToPreviousClip(const glm::mat4& previousCamera,
                                                             const glm::mat4& currentCamera);

} // namespace Render::PostProcessing

#include "Render/PostProcessing/TemporalJitter.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace Render::PostProcessing
{

namespace
{
constexpr std::uint32_t HorizontalBase = 2u;
constexpr std::uint32_t VerticalBase = 3u;
// Halton points lie in [0, 1); the offsets centre them on the pixel.
constexpr float PixelCentre = 0.5f;

// The `index`-th point (from 1) of the van der Corput sequence in `base`.
float RadicalInverse(std::uint32_t index, std::uint32_t base)
{
    float fraction = 1.0f;
    float result = 0.0f;
    while (index > 0u)
    {
        fraction /= static_cast<float>(base);
        result += fraction * static_cast<float>(index % base);
        index /= base;
    }
    return result;
}
} // namespace

glm::vec2 TemporalJitterPixels(std::uint32_t frameIndex)
{
    // Point 0 of the sequence is the corner (0, 0): the sequence is taken from point 1.
    const std::uint32_t point = frameIndex % TemporalJitterFrameCount + 1u;
    return {RadicalInverse(point, HorizontalBase) - PixelCentre, RadicalInverse(point, VerticalBase) - PixelCentre};
}

glm::vec2 TemporalJitterClip(glm::vec2 pixels, std::uint32_t width, std::uint32_t height)
{
    if (width == 0u || height == 0u)
    {
        return glm::vec2(0.0f);
    }
    // Clip space spans 2 across the target, and its y points up while pixel rows go down.
    return {2.0f * pixels.x / static_cast<float>(width), -2.0f * pixels.y / static_cast<float>(height)};
}

bool IsPerspective(const glm::mat4& transform)
{
    // The bottom row gives the projected w: an orthographic transform keeps it at 1.
    return transform[0][3] != 0.0f || transform[1][3] != 0.0f || transform[2][3] != 0.0f;
}

glm::mat4 JitterTransform(const glm::mat4& transform, glm::vec2 clipOffset)
{
    // The offset is added before the divide by w, so it is scaled by w here.
    return glm::translate(glm::mat4(1.0f), glm::vec3(clipOffset, 0.0f)) * transform;
}

std::optional<glm::mat4> CurrentToPreviousClip(const glm::mat4& previousCamera, const glm::mat4& currentCamera)
{
    // Double precision: world positions are thousands of units from the origin, and the product of
    // nearly equal cameras is close to the identity.
    const glm::dmat4 current(currentCamera);
    const double determinant = glm::determinant(current);
    if (!std::isfinite(determinant) || determinant == 0.0)
    {
        return std::nullopt;
    }
    return glm::mat4(glm::dmat4(previousCamera) * glm::inverse(current));
}

} // namespace Render::PostProcessing

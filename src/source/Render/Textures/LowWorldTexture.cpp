#include "Render/Textures/LowWorldTexture.h"

#include <algorithm>

namespace Render::Textures
{
MipLevel ReduceWorldTexture(std::span<const std::uint8_t> rgba, std::uint32_t width, std::uint32_t height,
                            std::uint8_t sourceScale, bool hasAlpha)
{
    constexpr std::uint32_t LowResolutionDivisor = 2;
    const auto divisor = std::max<std::uint32_t>(1, sourceScale) * LowResolutionDivisor;
    const auto targetWidth = std::max<std::uint32_t>(1, width / divisor);
    const auto targetHeight = std::max<std::uint32_t>(1, height / divisor);
    const auto alphaTest = AlphaTest::FromReference(AlphaTestReference);
    const float coverage = hasAlpha ? AlphaCoverage(rgba, width, height, alphaTest) : 0.f;
    MipLevel reduced = DownsampleBox(rgba, width, height);
    if (reduced.rgba.empty())
        return reduced;
    while (reduced.width > targetWidth || reduced.height > targetHeight)
    {
        reduced = DownsampleBox(reduced.rgba, reduced.width, reduced.height);
    }
    if (hasAlpha)
        PreserveAlphaCoverage(reduced, coverage, alphaTest);
    return reduced;
}
}

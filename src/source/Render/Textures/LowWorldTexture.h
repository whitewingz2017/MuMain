#pragma once

#include "Render/Textures/MipChain.h"

namespace Render::Textures
{
// Remove pack upscaling, then halve each world axis; logical UV dimensions stay unchanged.
[[nodiscard]] MipLevel ReduceWorldTexture(std::span<const std::uint8_t> rgba, std::uint32_t width,
                                         std::uint32_t height, std::uint8_t sourceScale, bool hasAlpha);
}

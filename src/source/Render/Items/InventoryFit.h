#pragma once

#include <algorithm>

namespace Render::Items::Display
{
// Fit a centered eye-space box, accounting for its front edge being nearer the camera.
inline float InventoryFitScale(float width, float height, float halfX, float halfY, float halfZ,
                               float depth, float perspectiveX, float perspectiveY)
{
    if (width <= 0 || height <= 0 || depth <= 0 || perspectiveX <= 0 || perspectiveY <= 0
        || halfX < 0 || halfY < 0 || halfZ < 0 || (halfX == 0 && halfY == 0)) return 0;
    constexpr float ContentFraction = 0.9f;
    const float x = width * ContentFraction * 0.5f * perspectiveX;
    const float y = height * ContentFraction * 0.5f * perspectiveY;
    const float scaleX = halfX + x * halfZ;
    const float scaleY = halfY + y * halfZ;
    if (scaleX == 0) return y * depth / scaleY;
    if (scaleY == 0) return x * depth / scaleX;
    return std::min(x * depth / scaleX, y * depth / scaleY);
}
}

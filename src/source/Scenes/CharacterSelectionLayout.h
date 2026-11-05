#pragma once

#include <array>

namespace Scenes
{
struct CharacterSelectionPose
{
    float x, y;
    float facing = 90.f;
};

// Keep creation and list refresh in the same forward-facing lineup.
inline constexpr std::array<CharacterSelectionPose, 5> CharacterSelectionPoses{{
    {8008.f, 18885.f}, {7986.f, 19145.f}, {8046.f, 19400.f},
    {8133.f, 19645.f}, {8282.f, 19845.f}
}};
}

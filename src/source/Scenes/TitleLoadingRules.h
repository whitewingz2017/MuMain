#pragma once
#include <cstdint>

namespace TitleSceneUI
{
inline constexpr int LoadingFrameCount = 15;
inline constexpr std::uint64_t LoadingFrameMs = 80;
constexpr float ProgressFraction(std::uint64_t now, std::uint64_t total)
{
    return total == 0 ? 0.f : now >= total ? 1.f : static_cast<float>(now) / static_cast<float>(total);
}
constexpr int LoadingFrame(std::uint64_t elapsedMs)
{
    return static_cast<int>((elapsedMs / LoadingFrameMs) % LoadingFrameCount);
}
}

#pragma once
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <numbers>

namespace UI::Chat
{
struct SystemMessageAnimation
{
    float opacity;
    float entryProgress;
    float exitProgress;
};
inline SystemMessageAnimation AnimateChatMessage(std::uint64_t ageMs)
{
    constexpr float EntryMs = 280.f;
    const float progress = std::clamp(static_cast<float>(ageMs) / EntryMs, 0.f, 1.f);
    const float eased = std::sin(progress * std::numbers::pi_v<float> / 2.f);
    return {eased, eased, 0.f};
}

inline SystemMessageAnimation AnimateSystemMessage(std::uint64_t ageMs)
{
    constexpr float LifetimeMs = 15000.f;
    constexpr float ExitMs = 500.f;
    auto state = AnimateChatMessage(ageMs);
    const float progress = std::clamp((static_cast<float>(ageMs) - LifetimeMs + ExitMs) / ExitMs, 0.f, 1.f);
    state.exitProgress = std::sin(progress * std::numbers::pi_v<float> / 2.f);
    state.opacity *= 1.f - state.exitProgress;
    return state;
}
}

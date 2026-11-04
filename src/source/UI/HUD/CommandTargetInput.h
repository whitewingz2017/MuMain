#pragma once

namespace UI::Commands
{
constexpr bool ShouldConfirmTarget(bool primaryPressed, bool secondaryPressed, bool hasTarget, bool overUi)
{
    return !overUi && (secondaryPressed || (primaryPressed && hasTarget));
}

static_assert(ShouldConfirmTarget(true, false, true, false));
static_assert(ShouldConfirmTarget(false, true, true, false));
static_assert(!ShouldConfirmTarget(true, false, false, false));
static_assert(!ShouldConfirmTarget(true, false, true, true));
static_assert(!ShouldConfirmTarget(false, true, true, true));
static_assert(ShouldConfirmTarget(false, true, false, false)); // Right-click still cancels in empty world.
} // namespace UI::Commands

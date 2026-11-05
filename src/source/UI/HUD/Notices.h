#pragma once

// Animated server, guild and event announcements, newest first above the HUD.
namespace UI::Notices
{
    // Push a notice; long lines wrap in the theme. Nonzero colour selects the guild tone.
    void Create(const wchar_t* text, int color);

    // Drop all current notices.
    void Clear();

    // Per-frame timing: advances the history after 15 idle seconds and retires outgoing text.
    void Move();

    // Draw the active notices.
    void Render();
}

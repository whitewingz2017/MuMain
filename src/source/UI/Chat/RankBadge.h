#pragma once

class CHARACTER;

// The badge of a player's rank title, over the name box (Data/Custom/RankTitles/RankTitles.json).
namespace UI::RankTitles::Badge
{
// Draws the badge centred over the name box that starts at (x, y) and is `width` wide, in interface units.
void Render(const CHARACTER* character, int x, int y, int width);

void RenderVisible();
} // namespace UI::RankTitles::Badge

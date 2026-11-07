#pragma once

class CHARACTER;
class OBJECT;

// The aura of a player's rank title, built from the layers of Data/Custom/RankTitles/RankTitles.json.
namespace Render::Effects::RankAura
{
// Emits the aura's particles; from the character effect update, never from the draw.
void Update(const CHARACTER* character, const OBJECT* object);
// The halos, before the character, so opaque armor and wings cover their centre.
void RenderBehind(const CHARACTER* character, const OBJECT* object);
// The other layers, after the character.
void Render(const CHARACTER* character, const OBJECT* object);
} // namespace Render::Effects::RankAura

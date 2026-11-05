#pragma once
class CHARACTER;
namespace GameLogic::Events
{
// The Blood Castle match state names the one character carrying the Archangel weapon (1..3), or none.
void SetArchangelCarrier(int key, int itemType);
// The weapon a character carries (1..3), 0 for none; the hero's own inventory is the truth for the hero.
int CarriedArchangelType(const CHARACTER* character);
}

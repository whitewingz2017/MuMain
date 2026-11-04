#pragma once
struct CHARACTER;
namespace GameLogic::Events
{
void SetArchangelCarrier(int key, int itemType);
int CarriedArchangelType(CHARACTER* character);
}

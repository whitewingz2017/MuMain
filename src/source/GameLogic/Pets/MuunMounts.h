#pragma once
class OBJECT;
class CHARACTER;
namespace GameLogic::Pets::MuunMounts
{
void Update(CHARACTER* character, int mainItem);
void EquipmentChanged(CHARACTER* character);
bool IsRiding(const OBJECT* owner, int item = -1);
int MovementHelper(const CHARACTER* character);
void Reset();
}

#pragma once

class CHARACTER;
class OBJECT;

namespace Render::Models::CosmeticArmor
{
void SetPreviewCharacter(CHARACTER* character, int itemType, int level = 0);
int WeaponModel(CHARACTER* character, int hand);
int SkillWeaponModel(const OBJECT* owner);
bool Render(CHARACTER* character, int selection = 0);
void WithEquipment(CHARACTER* character, OBJECT* object, int selection,
    void (*draw)(CHARACTER*, OBJECT*, int));
void Release();
}

#pragma once

#include <list>
#include <memory>

class PetObject;
class CHARACTER;

namespace GameLogic::Pets
{
// equipmentSlot: which equipment slot spawned the pet (TwinMU companion slot); every slot when allDelete.
void RemovePetObjects(std::list<std::shared_ptr<PetObject>>& pets, CHARACTER* owner, int itemType, bool allDelete,
                      int equipmentSlot = 0);
}

#pragma once

class CHARACTER;
class OBJECT;

namespace GameLogic::Pets::MountCombat
{
int OnAction(OBJECT* owner, int action);
void Update(CHARACTER* character);
bool IsFollowing(const OBJECT* owner);
int Helper(const CHARACTER* character);
void SetHelper(CHARACTER* character, int helper);
void Reset(CHARACTER* character);
}

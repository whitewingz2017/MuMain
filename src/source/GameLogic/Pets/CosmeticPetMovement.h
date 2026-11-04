#pragma once

class OBJECT;
namespace GameLogic::Pets
{
bool MoveGroundCosmetic(OBJECT* object, bool preview, bool grounded = true);
// S6 follower distances and actions; attackTarget is an observed server attack, not a damage request.
void MoveMuunFollower(OBJECT* object, double& idleSince, double now, bool floating, int attackTarget = -1);
}

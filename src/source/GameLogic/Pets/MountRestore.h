#pragma once
class OBJECT;
// Viewport packets create a player's mount once. Whatever later drops it without unequipping
// (a reused character slot, the owner briefly leaving play) is undone here: every mount created
// for a player is remembered until DeleteMount, and rebuilt while its owner is still present.
namespace GameLogic::Pets::MountRestore
{
void Remember(OBJECT* owner, int model);
void Forget(const OBJECT* owner);
void Update();
}

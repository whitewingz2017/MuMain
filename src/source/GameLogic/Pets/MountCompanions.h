#pragma once
class OBJECT;
namespace GameLogic::Pets::MountCompanions
{
bool InTown(const OBJECT* mount);
bool FollowInTown(OBJECT* mount, bool preview);
// The hero's mount is created only by viewport and inventory packets. A warp or a transformation
// change rebuilds the hero without one, so the equipped mount is recreated when the hero has none.
void EnsureHeroMount();
// A custom Fenrir mount rides exactly like a Horn of Fenrir: its rider is kept in the Fenrir
// helper state and Fenrir stand/walk/run actions while the mount carries it.
void SyncFenrirRider(const OBJECT* mount);
}

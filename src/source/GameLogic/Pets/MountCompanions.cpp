#include "stdafx.h"
#include "GameLogic/Pets/MountCompanions.h"
#include "GameLogic/Pets/MountRules.h"
#include "GameLogic/Pets/CosmeticPetMovement.h"
#include "Render/Terrain/ZzzLodTerrain.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInfomation.h"
#include "Engine/Object/ZzzObject.h"
#include "Engine/AI/GOBoid.h"
#include "GameLogic/Cosmetics/CosmeticArmor.h"
#include "GameLogic/Cosmetics/FenrirMounts.h"
#include "GameLogic/Pets/MuunMounts.h"
#include "GameLogic/Pets/MountCombat.h"
#include "Engine/Object/PlayerActionState.h"
#include "World/MapInfra/MapManager.h"
#include <SDL3/SDL.h>

namespace GameLogic::Pets::MountCompanions
{
namespace
{
double followerIdleSince[MAX_MOUNTS] = {};
bool RideableModel(int model)
{
    return model == MODEL_DARK_HORSE || model == MODEL_FENRIR_RED || model == MODEL_FENRIR_BLUE
        || model == MODEL_FENRIR_BLACK || model == MODEL_FENRIR_GOLD;
}

// The riding paths' own playback rates (GOBoid.cpp MoveMount): Fenrir stands at 0.4 and walks
// at 1.0; the Dark Horse plays its actions at 0.3.
float TownVelocity(const OBJECT& mount)
{
    constexpr int StandAction = 0;
    constexpr float FenrirStand = 0.4f, FenrirWalk = 1.f, DarkHorse = 0.3f;
    if (mount.Type == MODEL_DARK_HORSE) return DarkHorse;
    return mount.CurrentAction == StandAction ? FenrirStand : FenrirWalk;
}

// The model CreateEquippingEffect gives each helper; -1 for helpers that are not mounts.
int MountModel(int behaviorItem, int excellentFlags)
{
    constexpr int BlackFenrirFlag = 0x01, BlueFenrirFlag = 0x02, GoldFenrirFlag = 0x04;
    switch (behaviorItem)
    {
    case ITEM_HELPER: return MODEL_HELPER;
    case ITEM_HORN_OF_UNIRIA: return MODEL_UNICON;
    case ITEM_HORN_OF_DINORANT: return MODEL_PEGASUS;
    case ITEM_DARK_HORSE_ITEM: return MODEL_DARK_HORSE;
    case ITEM_HORN_OF_FENRIR:
        return excellentFlags == BlackFenrirFlag ? MODEL_FENRIR_BLACK : excellentFlags == BlueFenrirFlag ? MODEL_FENRIR_BLUE
            : excellentFlags == GoldFenrirFlag ? MODEL_FENRIR_GOLD : MODEL_FENRIR_RED;
    default: return -1;
    }
}

bool HasMount(const OBJECT* owner)
{
    for (int i = 0; i < MAX_MOUNTS; ++i)
        if (Mounts[i].Live && Mounts[i].Owner == owner) return true;
    return false;
}
}
void EnsureHeroMount()
{
    if (SceneFlag != MAIN_SCENE || !Hero || !CharacterMachine || !Hero->Object.Live || gMapManager.InChaosCastle()) return;
    const ITEM& helper = CharacterMachine->Equipment[EQUIPMENT_HELPER];
    const int behavior = helper.Type < 0 ? -1 : GameLogic::Cosmetics::HelperBehaviorItem(helper.Type);
    const int model = behavior < 0 ? -1 : MountModel(behavior, helper.ExcellentFlags);
    // Without a mount in the helper slot the hero has no mount. Some ways of taking one off left it
    // behind: it kept following in town and walked under the hero outside, who was not riding it.
    if (model < 0)
    {
        if (HasMount(&Hero->Object)) DeleteMount(&Hero->Object);
        return;
    }
    if (HasMount(&Hero->Object)) return;
    GameLogic::Cosmetics::FenrirMounts::SetItem(&Hero->Object, helper.Type);
    if (behavior == ITEM_HORN_OF_FENRIR) Hero->Helper.ExcellentFlags = helper.ExcellentFlags;
    CreateMount(model, Hero->Object.Position, &Hero->Object);
}
void SyncFenrirRider(const OBJECT* mount)
{
    if (!mount || !mount->Owner) return;
    if (!GameLogic::Cosmetics::IsFenrirMount(GameLogic::Cosmetics::FenrirMounts::Item(mount->Owner))) return;
    // A Muun ride owns the helper state while it lasts.
    if (MuunMounts::IsRiding(mount->Owner) || MountCombat::IsFollowing(mount->Owner)) return;
    CHARACTER* rider = CharacterOfObject(mount->Owner);
    if (!rider || rider->Object.Type != MODEL_PLAYER || rider->SafeZone) return;
    rider->Helper.Type = MODEL_HORN_OF_FENRIR;
    if (!Engine::Object::IsNonFenrirMovementAction(rider->Object.CurrentAction)) return;
    if (rider->Movement) SetPlayerWalk(rider);
    else SetPlayerStop(rider);
}
bool InTown(const OBJECT* mount)
{
    if (!mount || !mount->Owner || SceneFlag != MAIN_SCENE || !RideableModel(mount->Type)) return false;
    const auto* owner = mount->Owner;
    const int tile = TERRAIN_INDEX_REPEAT(static_cast<int>(owner->Position[0] / TERRAIN_SCALE), static_cast<int>(owner->Position[1] / TERRAIN_SCALE));
    return (TerrainWall[tile] & TW_SAFEZONE) == TW_SAFEZONE;
}
bool FollowInTown(OBJECT* mount, bool preview)
{
    const bool combatFollowing = MountCombat::IsFollowing(mount->Owner);
    const bool flyingMount = mount->Type == MODEL_UNICON || mount->Type == MODEL_PEGASUS;
    if (preview || SceneFlag != MAIN_SCENE || (!RideableModel(mount->Type) && !(flyingMount && combatFollowing))) return false;
    const float baseScale = mount->Type == MODEL_DARK_HORSE ? 1.f : 0.9f;
    mount->Scale = MountViewScale(baseScale, InTown(mount));
    // A transformed rider has no riding pose, so its mount walks beside it everywhere.
    const bool transformed = mount->Owner && mount->Owner->Type != MODEL_PLAYER;
    if (!InTown(mount) && !transformed && !combatFollowing) return false;
    mount->Alpha = 1.f;
    if (GameLogic::Cosmetics::IsFenrirMount(GameLogic::Cosmetics::FenrirMounts::Item(mount->Owner)))
    {
        const double now = static_cast<double>(SDL_GetTicks());
        double localIdleSince = now;
        double& idleSince = mount >= Mounts && mount < Mounts + MAX_MOUNTS
            ? followerIdleSince[mount - Mounts] : localIdleSince;
        MoveMuunFollower(mount, idleSince, now, false);
        return true;
    }
    MoveGroundCosmetic(mount, false, true);
    mount->Velocity = TownVelocity(*mount);
    return true;
}
}

#include "stdafx.h"
#include "GameLogic/Pets/Muun.h"
#include "GameLogic/Pets/CosmeticPetMovement.h"
#include "GameLogic/Cosmetics/CosmeticArmor.h"
#include "GameLogic/Cosmetics/CosmeticAnimationRules.h"
#include "Engine/AI/ZzzAI.h"
#include "Render/Models/ZzzBMD.h"
#include "Render/Terrain/ZzzLodTerrain.h"
#include "Engine/Object/ZzzObject.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInterface.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>

namespace GameLogic::Pets
{
namespace
{
void CosmeticFollowTarget(OBJECT* object, int muunType, vec3_t target)
{
    constexpr float SideDistance = 70.f;
    constexpr float BehindDistance = 50.f;
    // Fixed placements keep preview pets on opposite sides; ordinary helper mounts follow behind.
    constexpr float HelperMountBehindDistance = 130.f;
    const bool helperMount = object >= Mounts && object < Mounts + MAX_MOUNTS;
    const float side = !helperMount && Muun::IsType(muunType) ? Muun::FollowSide(object->Owner, muunType) : 1.f;
    vec3_t local{helperMount ? 0.f : SideDistance * side, helperMount ? HelperMountBehindDistance : BehindDistance, 0.f}, offset;
    float matrix[3][4];
    AngleMatrix(object->Owner->Angle, matrix);
    VectorRotate(local, matrix, offset);
    VectorAdd(object->Owner->Position, offset, target);
}

constexpr float FinalMuFollowRange = 150.f;
constexpr float FinalMuIdleRange = 70.f;
constexpr float FinalMuScatterDistance = 1000.f;
constexpr int FinalMuScatterSpread = 360;
constexpr int FinalMuStandAction = 0, FinalMuStepAction = 1, FinalMuRunAction = 2;
constexpr float FinalMuStandVelocity = 0.5f;

struct FinalMuSpeedBand
{
    float rangeFactor;
    float speed;
    float animationSpeed;
};
// Checked in order: beyond twice the range, beyond 1.5 times it, otherwise.
constexpr std::array<FinalMuSpeedBand, 3> FinalMuSpeedBands{{{2.f, 15.f, 0.89f}, {1.5f, 13.f, 0.69f}, {0.f, 10.f, 0.4f}}};

void SetFinalMuAction(OBJECT* object, int action)
{
    object->CurrentAction = static_cast<unsigned short>(action < Models[object->Type].NumActions ? action : FinalMuStandAction);
}

int FollowerMoveAction(const OBJECT* object)
{
    const bool customModel = GameLogic::Cosmetics::FindFinalMuSkin(object->Type - MODEL_ITEM) != nullptr;
    return customModel || !Muun::IsType(object->Type - MODEL_ITEM) ? FinalMuRunAction : FinalMuStepAction;
}

bool OwnerWalksOrRuns(const OBJECT* owner)
{
    const CHARACTER* character = CharacterOfObject(owner);
    return (character && character->Movement)
        || (owner->CurrentAction >= PLAYER_WALK_MALE && owner->CurrentAction <= PLAYER_RUN_RIDE_WEAPON);
}

const OBJECT* AttackTarget(const OBJECT* owner, int targetIndex)
{
    if (targetIndex < 0 && Hero && owner == &Hero->Object) targetIndex = SelectedCharacter;
    if (targetIndex < 0 || targetIndex >= MAX_CHARACTERS_CLIENT) return nullptr;
    const auto& target = CharactersClient[targetIndex];
    return target.Object.Live && !target.Dead && &target.Object != owner ? &target.Object : nullptr;
}

void StepTowardsOwner(OBJECT* object, float dx, float dy, float speed)
{
    const float distance = std::sqrt(dx * dx + dy * dy);
    if (distance <= 0.f) return;
    const float step = std::min(distance, speed * FPS_ANIMATION_FACTOR);
    object->Position[0] += dx / distance * step;
    object->Position[1] += dy / distance * step;
    object->Angle[2] = CreateAngle2D(object->Position, object->Owner->Position);
}

// Beyond the range: runs straight at the owner, faster the further away it is.
void CatchUp(OBJECT* object, float dx, float dy, float distanceSquared, float range)
{
    for (const auto& band : FinalMuSpeedBands)
    {
        const float bandRange = range * band.rangeFactor;
        if (distanceSquared < bandRange * bandRange) continue;
        object->Velocity = band.animationSpeed;
        SetFinalMuAction(object, FollowerMoveAction(object));
        StepTowardsOwner(object, dx, dy, band.speed);
        object->Direction[0] = object->Direction[1] = 0.f;
        return;
    }
}

// Starting at the owner's exact position still needs a direction, with main/sub pets on opposite sides.
void Wander(OBJECT* object, float dx, float dy)
{
    constexpr float DegreesToRadians = static_cast<float>(Q_PI) / 180.f;
    constexpr int WanderChance = 100, MinimumSpeedTenths = 16, SpeedVariationTenths = 64;
    constexpr int HeadingVariationDegrees = 90;
    if ((object->Direction[0] == 0.f && object->Direction[1] == 0.f) || rand_fps_check(WanderChance))
    {
        const float away = dx != 0.f || dy != 0.f ? std::atan2(-dy, -dx)
            : (object->Owner->Angle[2] + HeadingVariationDegrees * Muun::FollowSide(object->Owner, object->Type - MODEL_ITEM)) * DegreesToRadians;
        const float angle = away + (rand() % HeadingVariationDegrees - HeadingVariationDegrees / 2) * DegreesToRadians;
        const float speed = (MinimumSpeedTenths + rand() % SpeedVariationTenths) / 10.f;
        object->Direction[0] = std::cos(angle) * speed;
        object->Direction[1] = std::sin(angle) * speed;
    }
    object->Position[0] += object->Direction[0] * FPS_ANIMATION_FACTOR;
    object->Position[1] += object->Direction[1] * FPS_ANIMATION_FACTOR;
    object->Angle[2] = CreateAngle2D(object->Owner->Position, object->Position);
    object->Velocity = FinalMuSpeedBands.back().animationSpeed;
    SetFinalMuAction(object, FollowerMoveAction(object));
}

void Idle(OBJECT* object, double& idleSince, double now, const OBJECT* target)
{
    constexpr double TargetActionMs = 10000.0, IdleRefreshMs = 800.0;
    object->Velocity = FinalMuStandVelocity;
    SetFinalMuAction(object, FinalMuStandAction);
    if (!target) { if (now - idleSince > IdleRefreshMs) idleSince = now; return; }
    if (now - idleSince > TargetActionMs) { idleSince = now; return; }
    object->Angle[2] = CreateAngle2D(object->Position, target->Position);
    const int action = FollowerMoveAction(object) == FinalMuRunAction ? FinalMuStepAction : FinalMuRunAction;
    SetFinalMuAction(object, action);
}

// Left 1000 units behind: lands up to 360 units from the owner on each axis, on the side it was on.
void Scatter(OBJECT* object)
{
    for (int axis = 0; axis < 2; ++axis)
    {
        const float offset = static_cast<float>(rand() % FinalMuScatterSpread);
        const bool before = object->Position[axis] < object->Owner->Position[axis];
        object->Position[axis] = object->Owner->Position[axis] + (before ? -offset : offset);
    }
}
}

void MoveMuunFollower(OBJECT* object, double& idleSince, double now, bool floating, int attackTarget)
{
    if (!object || !object->Owner) return;
    const OBJECT* owner = object->Owner;
    const float gapX = owner->Position[0] - object->Position[0];
    const float gapY = owner->Position[1] - object->Position[1];
    if (gapX * gapX + gapY * gapY >= FinalMuScatterDistance * FinalMuScatterDistance) Scatter(object);
    const float dx = owner->Position[0] - object->Position[0];
    const float dy = owner->Position[1] - object->Position[1];
    const float distanceSquared = dx * dx + dy * dy;
    const float range = FinalMuFollowRange;
    if (distanceSquared >= range * range)
    {
        CatchUp(object, dx, dy, distanceSquared, range);
        idleSince = now;
    }
    else if (distanceSquared < FinalMuIdleRange * FinalMuIdleRange && !OwnerWalksOrRuns(owner))
    {
        Wander(object, dx, dy);
        idleSince = now;
    }
    else
    {
        object->Direction[0] = object->Direction[1] = 0.f;
        Idle(object, idleSince, now, AttackTarget(owner, attackTarget));
    }

    const int itemType = object->Type - MODEL_ITEM;
    object->Position[2] = owner->Position[2] + (floating ? GameLogic::Cosmetics::CosmeticPetHeight(itemType) : 0.f);
}

bool MoveGroundCosmetic(OBJECT* object, bool preview, bool grounded)
{
    constexpr float TeleportDistance = 500.f;
    constexpr float StopDistance = 15.f;
    constexpr float TurnSpeed = 10.f;
    const int muunType = object->Type == MODEL_DARK_HORSE ? DarkHorseItem
        : object->Type == MODEL_FENRIR_RED ? FenrirItem : object->Type - MODEL_ITEM;
    vec3_t target;
    CosmeticFollowTarget(object, muunType, target);
    const float dx = target[0] - object->Position[0];
    const float dy = target[1] - object->Position[1];
    const float distance = std::sqrt(dx * dx + dy * dy);
    const bool moving = distance > StopDistance;
    if (distance > TeleportDistance)
    {
        VectorCopy(target, object->Position);
    }
    else if (moving)
    {
        const float step = GameLogic::Cosmetics::GroundFollowStep(distance, FPS_ANIMATION_FACTOR);
        object->Position[0] += dx / distance * step;
        object->Position[1] += dy / distance * step;
        object->Angle[2] = TurnAngle2(object->Angle[2], CreateAngle2D(object->Position, target), TurnSpeed * FPS_ANIMATION_FACTOR);
    }
    const float groundHeight = preview || !grounded ? object->Owner->Position[2]
        : RequestTerrainHeight(object->Position[0], object->Position[1]);
    object->Position[2] = groundHeight + (grounded ? 0.f : GameLogic::Cosmetics::CosmeticPetHeight(muunType));
    object->CurrentAction = moving && Models[object->Type].NumActions > 1 ? 1 : 0;
    return true;
}
}

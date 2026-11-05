#include "stdafx.h"
#include "GameLogic/Pets/MountCombat.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/PlayerActionState.h"
#include "Character/CharacterManager.h"
#include "World/MapInfra/MapManager.h"
#include <SDL3/SDL.h>

namespace GameLogic::Pets::MountCombat
{
namespace
{
constexpr Uint64 RemountDelayMs = 800;

bool MountHelper(int helper)
{
    return helper == MODEL_HORN_OF_UNIRIA || helper == MODEL_HORN_OF_DINORANT
        || helper == MODEL_DARK_HORSE_ITEM || helper == MODEL_HORN_OF_FENRIR;
}

bool CombatAction(int action)
{
    return (action >= PLAYER_ATTACK_FIST && action < PLAYER_ATTACK_END)
        || (action >= PLAYER_ATTACK_STRIKE && action <= PLAYER_ATTACK_DARKHORSE)
        || (action >= PLAYER_FENRIR_ATTACK && action <= PLAYER_FENRIR_SKILL_ONE_LEFT)
        || (action >= PLAYER_ATTACK_BOW_UP && action <= PLAYER_ATTACK_STUN)
        || (action >= PLAYER_ATTACK_TWO_HAND_SWORD_TWO && action < PLAYER_DEFENSE1)
        || (action >= PLAYER_SKILL_THRUST && action <= PLAYER_RAGE_UNI_ATTACK_ONE_RIGHT)
        || (action >= PLAYER_RAGE_FENRIR && action <= PLAYER_RAGE_FENRIR_ONE_LEFT)
        || action == PLAYER_RAGE_FENRIR_ATTACK_RIGHT;
}

int GroundAttackAction(const CHARACTER* character, int action)
{
    switch (action)
    {
    case PLAYER_ATTACK_RIDE_SWORD:
    case PLAYER_ATTACK_RIDE_HORSE_SWORD:
    case PLAYER_FENRIR_ATTACK_ONE_SWORD:
    case PLAYER_FENRIR_ATTACK_TWO_SWORD:
    case PLAYER_FENRIR_ATTACK_DARKLORD_SWORD:
        return character->Weapon[0].Type != -1 ? PLAYER_ATTACK_SWORD_RIGHT1
            : character->Weapon[1].Type != -1 ? PLAYER_ATTACK_SWORD_LEFT1 : PLAYER_ATTACK_FIST;
    case PLAYER_ATTACK_RIDE_TWO_HAND_SWORD: return PLAYER_ATTACK_TWO_HAND_SWORD1;
    case PLAYER_ATTACK_RIDE_SPEAR:
    case PLAYER_FENRIR_ATTACK_SPEAR: return PLAYER_ATTACK_SPEAR1;
    case PLAYER_ATTACK_RIDE_SCYTHE: return PLAYER_ATTACK_SCYTHE1;
    case PLAYER_ATTACK_RIDE_BOW:
    case PLAYER_FENRIR_ATTACK_BOW: return character->Wing.Type != -1 ? PLAYER_ATTACK_FLY_BOW : PLAYER_ATTACK_BOW;
    case PLAYER_ATTACK_RIDE_CROSSBOW:
    case PLAYER_FENRIR_ATTACK_CROSSBOW: return character->Wing.Type != -1 ? PLAYER_ATTACK_FLY_CROSSBOW : PLAYER_ATTACK_CROSSBOW;
    case PLAYER_ATTACK_RIDE_BOW_UP: return character->Wing.Type != -1 ? PLAYER_ATTACK_FLY_BOW_UP : PLAYER_ATTACK_BOW_UP;
    case PLAYER_ATTACK_RIDE_CROSSBOW_UP: return character->Wing.Type != -1 ? PLAYER_ATTACK_FLY_CROSSBOW_UP : PLAYER_ATTACK_CROSSBOW_UP;
    case PLAYER_ATTACK_RIDE_STRIKE:
    case PLAYER_FENRIR_ATTACK_DARKLORD_STRIKE: return PLAYER_ATTACK_STRIKE;
    case PLAYER_ATTACK_RIDE_TELEPORT:
    case PLAYER_FENRIR_ATTACK_DARKLORD_TELEPORT: return PLAYER_ATTACK_TELEPORT;
    case PLAYER_ATTACK_RIDE_ATTACK_FLASH:
    case PLAYER_FENRIR_ATTACK_DARKLORD_FLASH: return PLAYER_ATTACK_ONE_FLASH;
    case PLAYER_FENRIR_ATTACK_DARKLORD_AQUA: return PLAYER_ATTACK_DEATH_CANNON;
    case PLAYER_FENRIR_ATTACK:
    case PLAYER_RAGE_UNI_ATTACK:
    case PLAYER_RAGE_UNI_ATTACK_ONE_RIGHT:
    case PLAYER_RAGE_FENRIR_ATTACK_RIGHT: return PLAYER_ATTACK_FIST;
    default: return action;
    }
}

int GroundSkillAction(const CHARACTER* character, int action)
{
    // Summoner skills store the foot, Uniria, Dinorant and Fenrir variants together.
    constexpr int SummonerVariants = 4;
    if (action >= PLAYER_SKILL_SLEEP && action <= PLAYER_SKILL_SUMMON_FENRIR)
        return action - (action - PLAYER_SKILL_SLEEP) % SummonerVariants;
    switch (action)
    {
    case PLAYER_SKILL_RIDER:
    case PLAYER_SKILL_RIDER_FLY:
    case PLAYER_RIDE_SKILL:
    case PLAYER_ATTACK_DARKHORSE:
    case PLAYER_ATTACK_RIDE_ATTACK_MAGIC:
    case PLAYER_FENRIR_ATTACK_MAGIC:
    case PLAYER_FENRIR_SKILL:
    case PLAYER_FENRIR_SKILL_TWO_SWORD:
    case PLAYER_FENRIR_SKILL_ONE_RIGHT:
    case PLAYER_FENRIR_SKILL_ONE_LEFT:
    case PLAYER_RAGE_FENRIR:
    case PLAYER_RAGE_FENRIR_TWO_SWORD:
    case PLAYER_RAGE_FENRIR_ONE_RIGHT:
    case PLAYER_RAGE_FENRIR_ONE_LEFT:
        return gCharacterManager.IsFemale(character->Class) ? PLAYER_SKILL_ELF1 : PLAYER_SKILL_HAND1;
#ifdef YDG_ADD_SKILL_RIDING_ANIMATIONS
    case PLAYER_SKILL_GIGANTICSTORM_UNI:
    case PLAYER_SKILL_GIGANTICSTORM_DINO:
    case PLAYER_SKILL_GIGANTICSTORM_FENRIR: return PLAYER_SKILL_GIGANTICSTORM;
    case PLAYER_ATTACK_SKILL_WHEEL_UNI:
    case PLAYER_ATTACK_SKILL_WHEEL_DINO:
    case PLAYER_ATTACK_SKILL_WHEEL_FENRIR: return PLAYER_ATTACK_SKILL_WHEEL;
#endif
    default: return action;
    }
}
}

int Helper(const CHARACTER* character)
{
    return character->MountResumeAt ? character->CombatMountHelper : character->Helper.Type;
}

void SetHelper(CHARACTER* character, int helper)
{
    if (character->MountResumeAt) character->CombatMountHelper = helper;
    character->Helper.Type = character->MountResumeAt && MountHelper(helper) ? -1 : helper;
}

void Reset(CHARACTER* character)
{
    character->MountResumeAt = 0;
    character->CombatMountHelper = -1;
    character->Helper.Type = -1;
}

bool IsFollowing(const OBJECT* owner)
{
    if (!owner || SceneFlag != MAIN_SCENE) return false;
    const auto* character = CharacterOfObject(owner);
    return character && character->MountResumeAt != 0 && MountHelper(Helper(character));
}

int OnAction(OBJECT* owner, int action)
{
    if (SceneFlag != MAIN_SCENE || owner->Type != MODEL_PLAYER || !owner->Live || !CombatAction(action)) return action;
    auto* character = CharacterOfObject(owner);
    if (!character || character->SafeZone || character->Dead || gMapManager.InChaosCastle()
        || !MountHelper(Helper(character))) return action;
    const int helper = Helper(character);
    character->MountResumeAt = SDL_GetTicks() + RemountDelayMs;
    SetHelper(character, helper);
    return GroundSkillAction(character, GroundAttackAction(character, action));
}

void Update(CHARACTER* character)
{
    if (!character->MountResumeAt) return;
    const auto now = SDL_GetTicks();
    if (SceneFlag == MAIN_SCENE && character->Object.Live)
    {
        if (character->Dead) return;
        if (CombatAction(character->Object.CurrentAction))
        {
            character->MountResumeAt = now + RemountDelayMs;
            return;
        }
        if (now < character->MountResumeAt) return;
    }
    const int helper = character->CombatMountHelper;
    character->MountResumeAt = 0;
    character->CombatMountHelper = -1;
    character->Helper.Type = helper;
    if (SceneFlag != MAIN_SCENE || !character->Object.Live || character->Object.Type != MODEL_PLAYER
        || character->SafeZone || gMapManager.InChaosCastle()
        || !Engine::Object::IsNonFenrirMovementAction(character->Object.CurrentAction)) return;
    if (character->Movement) SetPlayerWalk(character);
    else SetPlayerStop(character);
}
}

#pragma once

#include "Core/Globals/_enum.h"

namespace Engine::Object
{
    // True while a player's action is one of the attack/skill swing animations
    // (PLAYER_ATTACK_FIST .. PLAYER_RIDE_SKILL). The swing animation's playback
    // speed scales with AttackSpeed (see SetAttackSpeed in ZzzCharacter.cpp), so
    // this predicate doubles as the natural attack-cadence gate: hold off the
    // next action until the current swing finishes, and the rate follows attack
    // speed instead of any fixed timer.
    inline bool IsAttackAction(int currentAction)
    {
        return currentAction >= PLAYER_ATTACK_FIST && currentAction <= PLAYER_RIDE_SKILL;
    }

    // True while a player is sitting or holding a pose (PLAYER_SIT1 ..
    // PLAYER_POSE_FEMALE1). Used to keep these animations from being reset to the
    // stand pose when equipment or class changes.
    inline bool IsSitOrPoseAction(int currentAction)
    {
        return currentAction >= PLAYER_SIT1 && currentAction <= PLAYER_POSE_FEMALE1;
    }

    // True for the stand/walk/run animations SetPlayerStop and SetPlayerWalk pick when the
    // character is not on a Fenrir: on foot, flying, or on a Uniria, Dinorant or Dark Horse.
    inline bool IsNonFenrirMovementAction(int currentAction)
    {
        if (currentAction >= PLAYER_STOP_MALE && currentAction <= PLAYER_RUN_RIDE_WEAPON) return true;
        switch (currentAction)
        {
        case PLAYER_STOP_TWO_HAND_SWORD_TWO:
        case PLAYER_WALK_TWO_HAND_SWORD_TWO:
        case PLAYER_RUN_TWO_HAND_SWORD_TWO:
        case PLAYER_STOP_RAGEFIGHTER:
        case PLAYER_DARKLORD_STAND:
        case PLAYER_DARKLORD_WALK:
        case PLAYER_STOP_RIDE_HORSE:
        case PLAYER_RUN_RIDE_HORSE:
        case PLAYER_RAGE_UNI_RUN:
        case PLAYER_RAGE_UNI_RUN_ONE_RIGHT:
        case PLAYER_RAGE_UNI_STOP_ONE_RIGHT:
            return true;
        default:
            return false;
        }
    }
}

#include "stdafx.h"
#include "Render/Models/CosmeticAnimation.h"
#include "GameLogic/Cosmetics/CosmeticArmor.h"
#include "GameLogic/Cosmetics/CosmeticAnimationRules.h"
#include "Render/Models/ZzzBMD.h"
#include "Render/Items/ItemModelLookup.h"
#include "Engine/Object/w_CharacterInfo.h"
#include "GameLogic/Buffs/w_Buff.h"
#include <SDL3/SDL.h>

namespace Render::Models::CosmeticAnimation
{
void AdjustAttachment(int modelType, float parent[3][4])
{
    const auto* model = Render::Items::FindItemModel(modelType);
    if (!model || !model->equipment) return;
    const auto& display = *model->equipment;
    vec3_t angle;
    for (int axis = 0; axis < 3; ++axis) angle[axis] = static_cast<float>(display.rotation[axis]);
    float local[3][4];
    AngleMatrix(angle, local);
    for (int row = 0; row < 3; ++row)
    {
        for (int column = 0; column < 3; ++column)
            local[row][column] *= static_cast<float>(display.scale);
        local[row][3] = static_cast<float>(display.offset[row]);
    }
    float adjusted[3][4];
    R_ConcatTransforms(parent, local, adjusted);
    std::memcpy(parent, adjusted, sizeof adjusted);
}

namespace
{
bool AnimateEquipment(CHARACTER& character, _PART_t& part, int modelType, bool paused)
{
    const int skinPart = GameLogic::Cosmetics::ArmorPart(modelType - MODEL_ITEM);
    if (skinPart < 5 || skinPart > 7) return false;
    auto& model = ::Models[modelType];
    if (model.NumActions <= 0) return false;
    const auto& action = model.Actions[0];
    constexpr float ItemSpeed = 0.3f;
    const double time = static_cast<double>(SDL_GetTicks());
    const bool flying = character.Object.CurrentAction == PLAYER_FLY || character.Object.CurrentAction == PLAYER_FLY_CROSSBOW;
    const float phase = skinPart == 7
        ? character.CosmeticWingClock.Advance(modelType, time, action.NumAnimationKeys, action.LockPositions, flying, paused)
        : GameLogic::Cosmetics::AnimationPhase(time, action.NumAnimationKeys, action.LockPositions, ItemSpeed);
    const auto blend = GameLogic::Cosmetics::LoopAnimationBlend(phase, action.NumAnimationKeys - (action.LockPositions ? 1 : 0));
    model.CurrentAction = 0;
    part.CurrentAction = 0;
    part.PriorAction = 0;
    part.AnimationFrame = blend.current;
    part.PriorAnimationFrame = blend.prior;
    return true;
}
}

bool AnimateCosmeticPart(CHARACTER& character, _PART_t& part, int modelType)
{
    const auto& object = character.Object;
    const bool paused = object.m_BuffMap.isBuff(eDeBuff_Stun) || object.m_BuffMap.isBuff(eDeBuff_Sleep);
    return AnimateEquipment(character, part, modelType, paused);
}
}

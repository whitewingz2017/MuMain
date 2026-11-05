#include "stdafx.h"
#include "GameLogic/Pets/MountCompanions.h"
#include "GameLogic/Pets/MountRules.h"
#include "Render/Models/FenrirMount.h"
#include "GameLogic/Cosmetics/FenrirMounts.h"
#include "GameLogic/Cosmetics/CosmeticArmor.h"
#include "GameLogic/Cosmetics/CosmeticAnimationRules.h"
#include "Engine/Object/ZzzObject.h"
#include "Render/Models/ZzzBMD.h"
#include "Render/Items/ItemEffects.h"

namespace Render::Models::FenrirMount
{
namespace
{
struct SavedAppearance
{
    OBJECT& object;
    int type;
    float scale;
    unsigned short action;
    unsigned short priorAction;
    float frame;
    float prior;
    explicit SavedAppearance(OBJECT& value) : object(value), type(value.Type), scale(value.Scale),
        action(value.CurrentAction), priorAction(value.PriorAction), frame(value.AnimationFrame), prior(value.PriorAnimationFrame) {}
    ~SavedAppearance()
    {
        object.Type = type;
        object.Scale = scale;
        object.CurrentAction = action;
        object.PriorAction = priorAction;
        object.AnimationFrame = frame;
        object.PriorAnimationFrame = prior;
    }
};
}

bool Draw(OBJECT* mount, int state)
{
    const int item = GameLogic::Cosmetics::FenrirMounts::Item(mount->Owner);
    const auto* skin = GameLogic::Cosmetics::FindFinalMuSkin(item);
    if (!skin || !skin->mount) return false;
    auto& model = ::Models[MODEL_ITEM + item];
    const auto& original = ::Models[mount->Type];
    if (model.NumActions <= 0 || original.NumActions <= mount->CurrentAction) return false;
    const int targetAnimation = GameLogic::Cosmetics::MountAnimationAction(mount->CurrentAction, model.NumActions);
    const auto& sourceAction = original.Actions[mount->CurrentAction];
    const auto& targetAction = model.Actions[targetAnimation];
    const int sourceKeys = sourceAction.NumAnimationKeys - (sourceAction.LockPositions ? 1 : 0);
    const int targetKeys = targetAction.NumAnimationKeys - (targetAction.LockPositions ? 1 : 0);
    SavedAppearance restore(*mount);
    const auto blend = GameLogic::Cosmetics::MountAnimationBlend(mount->AnimationFrame, sourceKeys, targetKeys);
    mount->Type = MODEL_ITEM + item;
    // The ridden model's own scale (Dark Horse 1, Fenrir 0.9) carries the town and dismounted shrink.
    const float normalScale = restore.type == MODEL_DARK_HORSE ? 1.f : 0.9f;
    mount->Scale = GameLogic::Pets::MountCompanions::InTown(mount)
        ? GameLogic::Pets::MountViewScale(skin->scale, true) : skin->scale * restore.scale / normalScale;
    mount->CurrentAction = static_cast<unsigned short>(targetAnimation);
    mount->PriorAction = mount->CurrentAction;
    mount->AnimationFrame = blend.current;
    mount->PriorAnimationFrame = blend.prior;
    RenderObject(mount, false, 0, state);
    if (state == 0)
    {
        Render::Items::ItemEffects::RenderPetEffects(&model, mount, mount->Type, mount->Alpha,
            GameLogic::Pets::MountCompanions::InTown(mount));
    }
    return true;
}
}

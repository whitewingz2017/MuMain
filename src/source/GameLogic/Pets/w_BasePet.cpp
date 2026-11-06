// w_BasePet.cpp: implementation of the BasePet class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <SDL3/SDL.h>
#include <cmath>
#include "GameLogic/Pets/Muun.h"
#include "GameLogic/Pets/MuunMounts.h"
#include "GameLogic/Pets/MountCombat.h"
#include "Engine/AI/GOBoid.h"
#include "GameLogic/Pets/CosmeticPetMovement.h"
#include "GameLogic/Cosmetics/CosmeticArmor.h"
#include "GameLogic/Cosmetics/CosmeticAnimationRules.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Render/Items/ItemEffects.h"
#include "Render/Items/ScopedItemEffects.h"
#include "w_BasePet.h"
#include "Render/Terrain/ZzzLodTerrain.h"
#include "Engine/Object/ZzzObject.h"
#include "Engine/AI/ZzzAI.h"
#include "Render/Effects/ZzzEffect.h"
#include "Render/Renderer/MuRenderer.h"

extern float EarthQuake;

PetObjectPtr PetObject::Make()
{
    PetObjectPtr ptr(new PetObject());
    ptr->Init();
    return ptr;
}

PetObject::PetObject()
    : m_moveType(eAction_Stand), m_oldMoveType(eAction_End), m_targetKey(-1), m_pOwner(NULL), m_itemType(-1)
{
}

PetObject::~PetObject()
{
    Release();
}

void PetObject::Init()
{
    m_obj = new OBJECT();
}

void PetObject::SetScale(float scale)
{
    if (NULL == m_obj)
        return;

    m_baseScale = scale;
    m_obj->Scale = scale;
}

void PetObject::SetBlendMesh(int blendMesh)
{
    if (NULL == m_obj)
        return;

    m_obj->BlendMesh = blendMesh;
}

bool PetObject::Create(int itemType, int modelType, vec3_t Position, CHARACTER* Owner, int SubType, int LinkBone)
{
    assert(Owner);
    if (m_obj->Live)
        return FALSE;

    m_timer = new CTimer();

    m_pOwner = Owner;
    m_itemType = itemType;

    m_obj->Type = modelType;
    m_obj->Live = TRUE;
    m_obj->Visible = FALSE;
    m_obj->LightEnable = TRUE;
    m_obj->ContrastEnable = FALSE;
    m_obj->AlphaEnable = FALSE;
    m_obj->EnableBoneMatrix = FALSE;
    m_obj->Owner = &m_pOwner->Object;
    m_obj->SubType = SubType;
    m_obj->HiddenMesh = -1;
    m_obj->BlendMesh = -1;
    m_obj->BlendMeshLight = 1.f;
    m_obj->Scale = 0.7f;
    m_obj->LifeTime = 30;
    m_obj->Alpha = 0.f;
    m_obj->AlphaTarget = 1.f;

    VectorCopy(Position, m_obj->Position);
    VectorCopy(m_obj->Owner->Angle, m_obj->Angle);
    Vector(3.f, 3.f, 3.f, m_obj->Light);

    m_obj->PriorAnimationFrame = 0.f;
    m_obj->AnimationFrame = 0.f;
    m_obj->Velocity = 0.5f;

    switch (m_obj->Type)
    {
    case MODEL_PET_SKELETON:
        m_obj->Position[1] += (60.0f * Owner->Object.Scale);
        break;
    }
    return TRUE;
}

void PetObject::Release()
{
    if (NULL != m_obj)
    {
        m_obj->Live = FALSE;
        delete m_obj;
        m_obj = nullptr;
    }

    for (ActionMap::iterator iter = m_actionMap.begin(); iter != m_actionMap.end(); iter++)
    {
        Weak_Ptr(PetAction) petAction = (*iter).second;

        if (petAction.expired() == FALSE)
        {
            petAction.lock()->Release(m_obj, m_pOwner);
        }
    }
    m_actionMap.clear();

    m_speedMap.clear();

    if (m_timer)
    {
        delete m_timer;
        m_timer = nullptr;
    }
}

void PetObject::Update(bool bForceRender)
{
    if (!m_obj || !m_obj->Live || !m_obj->Owner)
        return;

    if (SceneFlag == MAIN_SCENE)
    {
        if (!m_obj->Owner->Live || m_obj->Owner->Kind != KIND_PLAYER)
        {
            m_obj->Live = FALSE;
            return;
        }
    }

    Alpha(m_obj);

    //-----------------------------//
    auto tick = m_timer->GetTimeElapsed();
    UpdateModel(tick, bForceRender);
    UpdateMove(tick, bForceRender);
    UpdateSound(tick, bForceRender);

    // For the pet the last frame drew; its sprites stay out of the bloom like its draw.
    if (m_obj->Visible)
    {
        const mu::ScopedBloomExclusion classicPets;
        UpdateDrawnObjectHold(m_obj);
        AnimateCosmetic();
        PlaceObjectModel(m_obj, &Models[m_obj->Type]);
        CreateEffect(tick, bForceRender);
    }
}

void PetObject::Cull()
{
    if (m_obj->Live)
        m_obj->Visible = TestFrustrum2D(m_obj->Position[0] * 0.01f, m_obj->Position[1] * 0.01f, -20.f);
}

void PetObject::Render(bool bForceRender) const
{
    // A forced pet draws wherever it stands.
    if (!m_obj->Live || (!bForceRender && !m_obj->Visible))
        return;
    const int State = g_isCharacterBuff(m_obj->Owner, eBuff_Cloaking) ? 10 : 0;
    RenderObject(m_obj, FALSE, 0, State);
    if (State == 0)
        RenderCosmeticEffects(bForceRender);
}

void PetObject::RenderCosmeticEffects(bool preview) const
{
    if (!GameLogic::Cosmetics::IsCosmeticPet(m_itemType) && !GameLogic::Pets::IsMountItem(m_itemType)) return;
    const auto* skin = GameLogic::Cosmetics::FindFinalMuSkin(m_itemType);
    const bool defaultGlow = skin && (skin->behavior == GameLogic::Cosmetics::FinalMuFollowerBehavior || m_pOwner->SafeZone);
    Render::Items::ScopedItemEffects scope(Render::Items::ScopedItemEffects::Enabled(),
        preview || Render::Items::ScopedItemEffects::LocalParticles());
    Render::Items::ItemEffects::RenderPetEffects(&Models[m_obj->Type], m_obj, m_obj->Type, m_obj->Alpha, defaultGlow);
}

bool PetObject::IsSameOwner(OBJECT* Owner)
{
    assert(Owner);

    return (Owner == m_obj->Owner) ? TRUE : FALSE;
}

bool PetObject::IsSameObject(OBJECT* Owner, int itemType)
{
    assert(Owner);

    return (Owner == m_obj->Owner && itemType == m_itemType) ? TRUE : FALSE;
}

void PetObject::SetActions(ActionType type, Weak_Ptr(PetAction) action, float speed)
{
    if (action.expired() == TRUE)
        return;

    m_actionMap.insert(std::make_pair(type, action));
    m_speedMap.insert(std::make_pair(type, speed));
}

void PetObject::SetCommand(int targetKey, ActionType cmdType)
{
    m_targetKey = targetKey;
    m_moveType = cmdType;
}

bool PetObject::UpdateMove(double tick, bool bForceRender)
{
    if (GameLogic::Pets::Muun::IsType(m_itemType))
    {
        UpdateMuunMovement(tick, bForceRender);
        return true;
    }
    if (m_oldMoveType != m_moveType)
    {
        m_oldMoveType = m_moveType;

        auto iter2 = m_speedMap.find(m_moveType);
        if (iter2 == m_speedMap.end())
            return FALSE;

        m_obj->Velocity = (*iter2).second;
    }

    auto iter = m_actionMap.find(m_moveType);
    if (iter == m_actionMap.end())
    {
        m_moveType = eAction_Stand;
        return FALSE;
    }

    // std::tr1::weak_ptr<PetAction> petAction = (*iter).second;

    Weak_Ptr(PetAction) petAction = (*iter).second;

    if (petAction.expired() == FALSE)
    {
        petAction.lock()->Move(m_obj, m_pOwner, m_targetKey, tick, bForceRender);
    }

    return TRUE;
}

void PetObject::UpdateRidingMount(bool preview)
{
    const int modelType = m_obj->Type;
    m_obj->Type = GameLogic::Pets::RidesAsDarkHorse(m_itemType) ? MODEL_DARK_HORSE : MODEL_FENRIR_RED;
    m_obj->AnimationFrame = m_mountFrame;
    m_obj->PriorAnimationFrame = m_mountPriorFrame;
    m_obj->CurrentAction = m_mountAction;
    m_obj->PriorAction = m_mountPriorAction;
    MoveMount(m_obj, preview);
    m_mountFrame = m_obj->AnimationFrame;
    m_mountPriorFrame = m_obj->PriorAnimationFrame;
    m_mountAction = m_obj->CurrentAction;
    m_mountPriorAction = m_obj->PriorAction;
    const auto& source = Models[m_obj->Type];
    m_obj->Type = modelType;
    m_obj->Scale = m_baseScale;
    const auto& target = Models[modelType];
    if (source.NumActions <= m_mountAction || target.NumActions <= 0) return;
    const int action = GameLogic::Cosmetics::MountAnimationAction(m_mountAction, target.NumActions);
    const auto& sourceAction = source.Actions[m_mountAction];
    const auto& targetAction = target.Actions[action];
    const auto blend = GameLogic::Cosmetics::MountAnimationBlend(m_mountFrame,
        sourceAction.NumAnimationKeys - (sourceAction.LockPositions ? 1 : 0),
        targetAction.NumAnimationKeys - (targetAction.LockPositions ? 1 : 0));
    m_obj->CurrentAction = static_cast<unsigned short>(action);
    m_obj->PriorAction = m_obj->CurrentAction;
    m_obj->AnimationFrame = blend.current;
    m_obj->PriorAnimationFrame = blend.prior;
}

void PetObject::UpdateMuunMovement(double tick, bool preview)
{
    // Dismounted to fight, a mount walks beside its rider at its safe-zone size.
    m_obj->Scale = GameLogic::Pets::IsMountItem(m_itemType)
        ? GameLogic::Pets::MountViewScale(m_baseScale,
            m_pOwner->SafeZone || GameLogic::Pets::MountCombat::IsFollowing(&m_pOwner->Object)) : m_baseScale;
    m_muunFollow = !preview;
    if (GameLogic::Pets::MuunMounts::IsRiding(&m_pOwner->Object, m_itemType))
    {
        UpdateRidingMount(preview);
        m_idleSince = tick;
        m_animatedAction = -1;
        return;
    }
    const bool floating = GameLogic::Cosmetics::IsFloatingPet(m_itemType);
    const auto* skin = GameLogic::Cosmetics::FindFinalMuSkin(m_itemType);
    if (m_muunFollow)
    {
        const int target = SDL_GetTicks() < m_muunAttackUntil ? m_muunTarget : -1;
        GameLogic::Pets::MoveMuunFollower(m_obj, m_idleSince, tick, floating, target);
        if (GameLogic::Pets::IsMountItem(m_itemType))
            m_obj->CurrentAction = GameLogic::Pets::MountFollowAction(m_obj->CurrentAction, Models[m_obj->Type].NumActions);
        return;
    }
    GameLogic::Pets::MoveGroundCosmetic(m_obj, preview, !floating);
    const int actionCount = Models[m_obj->Type].NumActions;
    if (skin && skin->behavior == GameLogic::Cosmetics::FinalMuFollowerBehavior)
    {
        const bool moving = m_obj->CurrentAction != 0;
        if (moving) m_idleSince = tick;
        m_obj->CurrentAction = static_cast<unsigned short>(
            GameLogic::Cosmetics::FinalMuFollowerAction(moving, tick - m_idleSince, actionCount));
        return;
    }
}

void PetObject::AnimateCosmetic()
{
    if ((!GameLogic::Cosmetics::IsCosmeticPet(m_itemType) && !GameLogic::Pets::Muun::IsType(m_itemType)) || !m_timer) return;
    if (GameLogic::Pets::MuunMounts::IsRiding(&m_pOwner->Object, m_itemType)) return;
    const auto& model = Models[m_obj->Type];
    if (model.NumActions <= 0) return;
    const int actionIndex = GameLogic::Cosmetics::IsFloatingPet(m_itemType)
        && !GameLogic::Cosmetics::FindFinalMuSkin(m_itemType) ? 0
        : std::min<int>(m_obj->CurrentAction, model.NumActions - 1);
    const auto& action = model.Actions[actionIndex];
    constexpr float CosmeticSpeed = 0.3f;
    const float speed = m_muunFollow ? m_obj->Velocity : CosmeticSpeed;
    const double tick = m_timer->GetTimeElapsed();
    const int cycleKeys = action.NumAnimationKeys - (action.LockPositions ? 1 : 0);
    if (m_animatedAction != actionIndex)
    {
        m_animatedAction = actionIndex;
        m_animationPhase = 0.f;
    }
    else if (cycleKeys > 1)
    {
        // Accumulated, so a speed change does not jump the animation.
        constexpr double NormalFramesPerSecond = 25.0, MillisecondsPerSecond = 1000.0;
        const double advance = (tick - m_animationTick) * NormalFramesPerSecond * speed / MillisecondsPerSecond;
        m_animationPhase = static_cast<float>(std::fmod(m_animationPhase + advance, static_cast<double>(cycleKeys)));
    }
    m_animationTick = tick;
    const auto blend = GameLogic::Cosmetics::LoopAnimationBlend(m_animationPhase, cycleKeys);
    m_obj->CurrentAction = static_cast<unsigned short>(actionIndex);
    m_obj->PriorAction = m_obj->CurrentAction;
    m_obj->AnimationFrame = blend.current;
    m_obj->PriorAnimationFrame = blend.prior;
}

bool PetObject::UpdateModel(double tick, bool bForceRender)
{
    auto iter = m_actionMap.find(m_moveType);
    if (iter == m_actionMap.end())
        return FALSE;

    Weak_Ptr(PetAction) petAction = (*iter).second;

    if (petAction.expired() == FALSE)
    {
        petAction.lock()->Model(m_obj, m_pOwner, m_targetKey, tick, bForceRender);
    }

    if (GameLogic::Cosmetics::IsCosmeticPet(m_itemType) || GameLogic::Pets::Muun::IsType(m_itemType)) return TRUE;
    BMD* b = &Models[m_obj->Type];
    b->CurrentAction = m_obj->CurrentAction;
    b->PlayAnimation(&m_obj->AnimationFrame, &m_obj->PriorAnimationFrame, &m_obj->PriorAction, m_obj->Velocity,
                     m_obj->Position, m_obj->Angle);

    return TRUE;
}

bool PetObject::UpdateSound(double tick, bool bForceRender)
{
    auto iter = m_actionMap.find(m_moveType);
    if (iter == m_actionMap.end())
        return FALSE;

    Weak_Ptr(PetAction) petAction = (*iter).second;

    if (petAction.expired() == FALSE)
    {
        petAction.lock()->Sound(m_obj, m_pOwner, m_targetKey, tick, bForceRender);
    }

    return TRUE;
}

bool PetObject::CreateEffect(double tick, bool bForceRender)
{
    auto iter = m_actionMap.find(m_moveType);
    if (iter == m_actionMap.end())
        return FALSE;

    Weak_Ptr(PetAction) petAction = (*iter).second;

    if (petAction.expired() == FALSE)
    {
        petAction.lock()->Effect(m_obj, m_pOwner, m_targetKey, tick, bForceRender);
    }

    return TRUE;
}

void PetObject::ShowMuunAttack(int targetIndex)
{
    if (!GameLogic::Pets::Muun::IsType(m_itemType) || targetIndex < 0 || targetIndex >= MAX_CHARACTERS_CLIENT) return;
    m_muunTarget = targetIndex; m_muunAttackUntil = SDL_GetTicks() + 600;
}

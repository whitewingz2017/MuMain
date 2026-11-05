#pragma once

// Relative path lets standalone cppcheck resolve the pointer macros without global header analysis.
#include "../../Core/Globals/_define.h"
#include "w_PetAction.h"
#include "Render/Models/ZzzBMD.h"
#include "Core/Time/Timer.h"

SmartPointer(PetObject);
class PetObject
{
public:
    enum ActionType {
        eAction_Stand = 0,
        eAction_Move,
        eAction_Attack,
        eAction_Skill,
        eAction_Birth,
        eAction_Dead,
        eAction_End
    };

public:
    static PetObjectPtr Make();
    virtual ~PetObject();

public:
    OBJECT* GetObject() { return m_obj; }
    bool IsSameOwner(OBJECT* Owner);
    bool IsSameObject(OBJECT* Owner, int itemType);
    // Update kills a pet whose owner vanished (warp, scope exit, transform rebuild); the owner slot is reused.
    bool IsAlive() const { return m_obj && m_obj->Live; }
    void SetActions(ActionType type, Weak_Ptr(PetAction) action, float speed);
    void SetCommand(int targetKey, ActionType cmdType);

    void ShowMuunAttack(int targetIndex);
    void SetScale(float scale = 0.0f);
    void SetBlendMesh(int blendMesh = -1);

public:
    bool Create(int itemType, int modelType, vec3_t Position, CHARACTER* Owner, int SubType, int LinkBone);
    void Release();
    void Update(bool bForceRender = false);
    // Whether the frame's view shows it; part of CullWorld.
    void Cull();
    void Render(bool bForceRender = false) const;

private:
    bool UpdateMove(double tick, bool bForceRender = false);
    bool UpdateModel(double tick, bool bForceRender = false);
    void AnimateCosmetic();
    void UpdateMuunMovement(double tick, bool preview);
    void UpdateRidingMount(bool preview);
    void RenderCosmeticEffects(bool preview) const;
    bool UpdateSound(double tick, bool bForceRender = false);

    bool CreateEffect(double tick, bool bForceRender = false);

private:
    void Init();
    PetObject();

public:
    typedef std::map< ActionType, Weak_Ptr(PetAction) > ActionMap;
    typedef std::map< ActionType, float > SpeedMap;

private:
    ActionMap m_actionMap;
    SpeedMap m_speedMap;

    CHARACTER* m_pOwner;
    OBJECT* m_obj;
    int m_targetKey;
    int m_itemType;
    float m_baseScale = 0.7f;
    unsigned long long m_muunAttackUntil = 0;
    int m_muunTarget = -1;
    double m_idleSince = 0.0;
    double m_animationTick = 0.0;
    float m_animationPhase = 0.f;
    bool m_muunFollow = false;
    int m_animatedAction = -1;
    float m_mountFrame = 0.f;
    float m_mountPriorFrame = 0.f;
    unsigned short m_mountAction = 0;
    unsigned short m_mountPriorAction = 0;

    CTimer* m_timer = nullptr;

    ActionType m_moveType;
    ActionType m_oldMoveType;
};

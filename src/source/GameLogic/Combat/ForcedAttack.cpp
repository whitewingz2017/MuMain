#include "stdafx.h"
#include "GameLogic/Combat/ForcedAttack.h"
#include "Core/Input/KeyState.h"

namespace GameLogic::Combat
{
    bool IsForcedAttackRequested()
    {
        if (Core::Input::IsKeyDown(VK_CONTROL)) return true;
        return false;
    }
}

#include "stdafx.h"
#include "GameLogic/Combat/ScopedGroundAim.h"
namespace GameLogic::Combat
{
namespace { thread_local CHARACTER* aimedCharacter = nullptr; }
ScopedGroundAim::ScopedGroundAim(CHARACTER* character) : m_Previous(aimedCharacter) { aimedCharacter = character; }
ScopedGroundAim::~ScopedGroundAim() { aimedCharacter = m_Previous; }
bool HasGroundAim(const CHARACTER* character) { return character && aimedCharacter == character; }
}

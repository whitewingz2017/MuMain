#include "stdafx.h"
#include "GameLogic/Pets/MountRestore.h"
#include "GameLogic/Cosmetics/FenrirMounts.h"
#include "Engine/AI/GOBoid.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzObject.h"
#include "World/MapInfra/MapManager.h"
#include <SDL3/SDL.h>
#include <string>
#include <unordered_map>

namespace GameLogic::Pets::MountRestore
{
namespace
{
constexpr Uint64 CheckIntervalMs = 250;
struct Expected
{
    CHARACTER* character;
    int key;
    std::wstring name;
    int model;
    int skin;
};
std::unordered_map<const OBJECT*, Expected> expected;
Uint64 nextCheck = 0;
bool restoring = false;

CHARACTER* OwnerCharacter(OBJECT* owner)
{
    for (int i = 0; i < MAX_CHARACTERS_CLIENT; ++i)
        if (&CharactersClient[i].Object == owner) return &CharactersClient[i];
    return nullptr;
}

bool HasMount(const OBJECT* owner)
{
    for (int i = 0; i < MAX_MOUNTS; ++i)
        if (Mounts[i].Live && Mounts[i].Owner == owner) return true;
    return false;
}

bool SameOwner(const Expected& entry)
{
    return entry.character->Key == entry.key && entry.name == entry.character->ID;
}
}

void Remember(OBJECT* owner, int model)
{
    if (restoring || !owner) return;
    auto* character = OwnerCharacter(owner);
    if (!character) return;
    expected[owner] = {character, character->Key, character->ID, model, GameLogic::Cosmetics::FenrirMounts::Item(owner)};
}

void Forget(const OBJECT* owner) { expected.erase(owner); }

void Update()
{
    if (SceneFlag != MAIN_SCENE || SDL_GetTicks() < nextCheck) return;
    nextCheck = SDL_GetTicks() + CheckIntervalMs;
    if (gMapManager.InChaosCastle()) return;
    for (auto entry = expected.begin(); entry != expected.end();)
    {
        auto* owner = &entry->second.character->Object;
        // A reused slot belongs to someone else now; that player's own packets bring its mount.
        if (!SameOwner(entry->second)) { entry = expected.erase(entry); continue; }
        if (owner->Live && owner->Kind == KIND_PLAYER && !HasMount(owner))
        {
            restoring = true;
            GameLogic::Cosmetics::FenrirMounts::SetItem(owner, entry->second.skin);
            CreateMount(entry->second.model, owner->Position, owner);
            restoring = false;
        }
        ++entry;
    }
}
}

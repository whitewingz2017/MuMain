#include "stdafx.h"
#include "GameLogic/Pets/Muun.h"
#include "GameLogic/Pets/MuunDrop.h"
#include "GameLogic/Pets/MuunMounts.h"
#include "GameLogic/Pets/w_PetProcess.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Network/Server/SocketSystem.h"
#include "Network/Server/WSclient.h"
#include "GameLogic/Pets/MuunPackets.h"
#include "Core/Utilities/StringUtils.h"
#include <SDL3/SDL.h>
#include <array>
#include <unordered_map>

namespace GameLogic::Pets::Muun
{
namespace
{
constexpr int RequestSize = 11;
constexpr Uint64 TimeoutMs = 8000, AppearanceIntervalMs = 100;
State state;
std::uint32_t sequence = 0, pendingRequest = 0;
std::optional<DropRequest> pendingEquip;
Uint64 sentAt = 0, nextAppearanceCheck = 0;
std::wstring owner;
struct Appearance { std::wstring name; std::array<int, 2> types{-1, -1}; };
std::unordered_map<int, Appearance> appearances;
std::unordered_map<const OBJECT*, std::array<int, 2>> activeCompanions;
bool sessionRequested = false;
using Wire::Payload;
using Wire::Integer;
}
const State& Get() { return state; }
const Species* Rule(int type)
{
    for (const auto& rule : state.species) if (rule.type == type) return &rule;
    return nullptr;
}
void Reset()
{
    for (const auto& [id, appearance] : appearances)
    {
        const int index = FindCharacterIndex(id);
        if (index < 0 || index >= MAX_CHARACTERS_CLIENT) continue;
        auto* character = &CharactersClient[index];
        if (appearance.name != character->ID) continue;
        for (const int type : appearance.types) if (IsType(type)) ThePetProcess().DeletePet(character, type, false);
    }
    MuunMounts::Reset();
    appearances.clear(); activeCompanions.clear(); owner.clear(); pendingRequest = 0; sessionRequested = false; pendingEquip.reset();
    const auto revision = state.revision + 1;
    state = {}; state.revision = revision;
}
bool Request(std::uint8_t operation, int source, int target)
{
    if (!SocketClient || !SocketClient->IsConnected() || SceneFlag != MAIN_SCENE || !Hero
        || operation > LastOperation || source < 0 || source > 255 || target < 0 || target > 255 || state.pending) return false;
    if (!owner.empty() && owner != Hero->ID) Reset();
    owner = Hero->ID;
    pendingRequest = ++sequence;
    if (!pendingRequest) pendingRequest = ++sequence;
    state.pending = true; sessionRequested = true; sentAt = SDL_GetTicks();
    BYTE packet[RequestSize] = {0xC1, RequestSize, 0xFD, 0x05, operation, static_cast<BYTE>(source), static_cast<BYTE>(target)};
    for (int i = 0; i < 4; ++i) packet[7 + i] = (pendingRequest >> (i * 8)) & 255;
    SocketClient->Send(packet, RequestSize); return true;
}
bool RequestDrop(const DropRequest& request)
{
    const int slot = FreeCollectionSlot(state);
    if (request.equipment >= 0 && (request.operation != 1 || request.equipment > 1 || !IsType(request.type) || slot < 0)) return false;
    if (!Request(request.operation, request.source, request.target)) return false;
    if (request.equipment >= 0) pendingEquip = DropRequest{3, slot, request.equipment, -1, request.type};
    return true;
}
void Receive(std::span<const std::uint8_t> packet)
{
    if (!Hero || owner != Hero->ID || SceneFlag != MAIN_SCENE) return;
    State next;
    if (!Wire::Snapshot(packet, pendingRequest, next)) return;
    const auto equip = pendingEquip;
    pendingEquip.reset();
    // Older servers deposit first; equip only the newly acknowledged pet, never after a timeout.
    const bool collected = equip && next.items.size() == state.items.size() + 1
        && std::none_of(state.items.begin(), state.items.end(), [&equip](const auto& item) { return item.slot == equip->source; })
        && std::any_of(next.items.begin(), next.items.end(), [&equip](const auto& item) { return item.slot == equip->source && item.type == equip->type; });
    next.revision = state.revision + 1; state = std::move(next); pendingRequest = 0;
    if (collected) Request(equip->operation, equip->source, equip->target);
}

void ReceiveAppearance(std::span<const std::uint8_t> packet)
{
    const auto json = Payload(packet, 6);
    if (!json.is_object() || SceneFlag != MAIN_SCENE || !Integer(json, "Id", 0, 32767)
        || !json.contains("Name") || !json["Name"].is_string()) return;
    const auto name = json["Name"].get<std::string>();
    if (name.empty() || name.size() > 40) return;
    if (json.contains("AttackTarget")) {
        if (!Integer(json, "AttackTarget", 0, 32767) || !Integer(json, "MuunSlot", 0, 1)) return;
        const int ownerIndex = FindCharacterIndex(json["Id"]);
        const int targetIndex = FindCharacterIndex(json["AttackTarget"]);
        const auto found = appearances.find(json["Id"]);
        if (ownerIndex < 0 || ownerIndex >= MAX_CHARACTERS_CLIENT || targetIndex < 0 || targetIndex >= MAX_CHARACTERS_CLIENT
            || found == appearances.end() || found->second.name != StringUtils::NarrowToWide(name)
            || CharactersClient[ownerIndex].ID != found->second.name) return;
        ThePetProcess().AttackMuun(&CharactersClient[ownerIndex], found->second.types[json["MuunSlot"].get<int>()], targetIndex);
        return;
    }
    if (!json.contains("Pets") || !json["Pets"].is_array() || json["Pets"].size() != 2) return;
    Appearance next; next.name = StringUtils::NarrowToWide(name);
    for (int i = 0; i < 2; ++i)
    {
        const auto& type = json["Pets"][i];
        if (!type.is_number_integer() || type < -1 || type > 8191 || (type != -1 && !IsType(type.get<int>()))) return;
        next.types[i] = type.get<int>();
    }
    const int id = json["Id"];
    const int index = FindCharacterIndex(id);
    if (const auto old = appearances.find(id); old != appearances.end() && index >= 0 && index < MAX_CHARACTERS_CLIENT)
    {
        auto* character = &CharactersClient[index];
        if (old->second.name == character->ID)
            for (const int type : old->second.types)
                if (IsType(type) && std::find(next.types.begin(), next.types.end(), type) == next.types.end())
                    ThePetProcess().DeletePet(character, type, false);
    }
    if (appearances.size() < MAX_CHARACTERS_CLIENT || appearances.contains(id)) appearances[id] = std::move(next);
}
float FollowSide(const OBJECT* object, int type)
{
    const auto found = activeCompanions.find(object);
    if (found != activeCompanions.end()) return found->second[1] == type ? -1.f : 1.f;
    return 1.f;
}
void Update()
{
    if (SceneFlag != MAIN_SCENE || !Hero) { if (!owner.empty() || !appearances.empty()) Reset(); return; }
    if (!owner.empty() && owner != Hero->ID) Reset();
    owner = Hero->ID;
    if (!sessionRequested) Request();
    if (state.pending && SDL_GetTicks() - sentAt >= TimeoutMs)
    {
        state.pending = false; pendingRequest = 0; pendingEquip.reset(); state.status = "Muun request timed out. Refresh to retry."; ++state.revision;
    }
    // Riding follows safe-zone transitions every frame, not only inventory refreshes.
    for (const auto& [id, appearance] : appearances)
    {
        const int index = FindCharacterIndex(id);
        if (index >= 0 && index < MAX_CHARACTERS_CLIENT && CharactersClient[index].ID == appearance.name)
            MuunMounts::Update(&CharactersClient[index], appearance.types[0]);
    }
    if (SDL_GetTicks() < nextAppearanceCheck) return;
    nextAppearanceCheck = SDL_GetTicks() + AppearanceIntervalMs;
    activeCompanions.clear();
    for (const auto& [id, appearance] : appearances)
    {
        const int index = FindCharacterIndex(id);
        if (index < 0 || index >= MAX_CHARACTERS_CLIENT) continue;
        auto* character = &CharactersClient[index];
        if (!character->Object.Live || character->ID != appearance.name) continue;
        activeCompanions[&character->Object] = appearance.types;
        for (const int type : appearance.types)
            if (IsType(type) && !ThePetProcess().HasPet(character, type))
                ThePetProcess().CreatePet(type, type == DarkHorseItem ? MODEL_DARK_HORSE : type == FenrirItem ? MODEL_FENRIR_RED : MODEL_ITEM + type, character->Object.Position, character);
    }
}
}

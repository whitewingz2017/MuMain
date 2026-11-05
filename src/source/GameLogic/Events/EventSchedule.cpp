#include "stdafx.h"
#include "GameLogic/Events/EventSchedule.h"
#include "GameLogic/Events/EventScheduleSnapshot.h"
#include "UI/Events/EventScheduleWindow.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Dotnet/Connection.h"
#include "Network/Server/WSclient.h"
#include "Scenes/SceneCommon.h"
#include "UI/Chat/ChatMessages.h"
#include <SDL3/SDL.h>

namespace GameLogic::Events::Schedule
{
namespace
{
std::uint32_t sequence = 0, pending = 0;
std::wstring owner;
std::uint64_t sentAt = 0;
constexpr std::uint64_t RequestTimeoutMs = 10000;
}
void Open()
{
    if (!SocketClient || !SocketClient->IsConnected() || !Hero || SceneFlag != MAIN_SCENE) return;
    if (pending && owner == Hero->ID && SDL_GetTicks() - sentAt < RequestTimeoutMs) return;
    pending = ++sequence;
    if (!pending) pending = ++sequence;
    owner = Hero->ID;
    sentAt = SDL_GetTicks();
    BYTE packet[8] = {0xC1, 8, 0xFB, 0x20};
    for (int i = 0; i < 4; ++i) packet[4 + i] = (pending >> (8 * i)) & 255;
    SocketClient->Send(packet, sizeof(packet));
}
void Update()
{
    if (!pending) return;
    if (!Hero || SceneFlag != MAIN_SCENE || owner != Hero->ID) { pending = 0; return; }
    if (SDL_GetTicks() - sentAt < RequestTimeoutMs) return;
    pending = 0;
    UI::Chat::PostSystem(L"Event timer request timed out. Reopen Event Timers to retry.", mu::ui::window::TYPE_ERROR_MESSAGE);
}
void Receive(std::span<const std::uint8_t> packet)
{
    Snapshot snapshot;
    if (!pending || !Parse(packet, snapshot) || snapshot.requestId != pending) return;
    pending = 0;
    if (Hero && SceneFlag == MAIN_SCENE && owner == Hero->ID) UI::Events::ShowSchedule(snapshot);
}
}

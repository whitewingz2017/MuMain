#include "stdafx.h"
#include "GameLogic/Bank/JewelBank.h"
#include "UI/Bank/JewelBank.h"
#include "Dotnet/Connection.h"
#include "Network/Server/WSclient.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Scenes/SceneCommon.h"
#include "UI/Core/WindowSystem.h"
#include "UI/Chat/ChatMessages.h"
#include <SDL3/SDL.h>
namespace GameLogic::Bank
{
namespace
{
std::uint32_t sequence = 0, pending = 0;
Uint64 sentAt = 0;
std::wstring owner;
constexpr Uint64 RequestTimeoutMs = 8000;
}
bool Request(std::uint8_t operation, std::uint16_t item, std::uint32_t amount)
{
    if (!SocketClient || !SocketClient->IsConnected() || SceneFlag != MAIN_SCENE || !Hero || operation > 2) return false;
    if (operation != 0 && g_pMyShopInventory && g_pMyShopInventory->IsEnablePersonalShop())
    {
        UI::Chat::PostSystem(L"You cannot deposit or withdraw jewels while your personal store is open.", mu::ui::window::TYPE_ERROR_MESSAGE);
        return true;
    }
    // Reopening while the same character waits must not report a lost connection or resend money operations.
    if (pending && owner == Hero->ID && SDL_GetTicks() - sentAt < RequestTimeoutMs) return true;
    pending = ++sequence;
    if (!pending) pending = ++sequence;
    owner = Hero->ID;
    sentAt = SDL_GetTicks();
    BYTE packet[15] = {0xC1, 15, 0xFA, 0x20, operation};
    packet[5] = item & 255; packet[6] = item >> 8;
    for (int i = 0; i < 4; ++i) { packet[7+i] = (amount >> (8*i)) & 255; packet[11+i] = (pending >> (8*i)) & 255; }
    SocketClient->Send(packet, sizeof(packet));
    return true;
}
void Receive(std::span<const std::uint8_t> packet)
{
    Snapshot snapshot;
    if (!pending || !ParseSnapshot(packet, snapshot) || snapshot.requestId != pending) return;
    pending = 0;
    if (SceneFlag == MAIN_SCENE && Hero && owner == Hero->ID) UI::Bank::Show(snapshot);
}
}

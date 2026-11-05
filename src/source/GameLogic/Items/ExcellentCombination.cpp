#include "stdafx.h"
#include "GameLogic/Items/ExcellentCombination.h"
#include "Network/Server/SocketSystem.h"
#include "Dotnet/Connection.h"
#include "Network/Server/WSclient.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Engine/Object/ZzzInfomation.h"
#include <SDL3/SDL.h>
#include "json.hpp"
namespace GameLogic::Items::ExcellentCombination {
namespace { State state; std::uint32_t sequence=0, pending=0; Uint64 sentAt=0; std::wstring owner; constexpr Uint64 TimeoutMs=8000; }
const State& Get() { return state; }
bool IsEligible(const ITEM* item) {
    constexpr int ExcellentOptionMask = 63;
    return item && item->Type >= 0 && item->Type < MAX_ITEM
        && ItemAttribute[item->Type].m_byItemSlot <= EQUIPMENT_RING_LEFT
        && (item->ExcellentFlags & ExcellentOptionMask) != 0
        && item->AncientDiscriminator == 0 && item->SocketCount == 0
        && !g_SocketItemMgr.IsSocketItem(item);
}
void Clear() { pending=0; state.pending=false; state.valid=false; state.success=false; state.status.clear(); ++state.revision; }
bool Request(bool combine, int primary, int secondary) {
    if (state.pending || !SocketClient || !SocketClient->IsConnected() || SceneFlag!=MAIN_SCENE || !Hero || primary<0 || primary>255 || secondary<0 || secondary>255) return false;
    if (combine && !state.valid) return false;
    pending=++sequence; if (!pending) pending=++sequence;
    owner=Hero->ID; sentAt=SDL_GetTicks();
    state.pending=true; state.valid=false; state.success=false; state.status=combine ? "Combining..." : "Checking items..."; ++state.revision;
    BYTE packet[11]={0xC1,11,0xFD,3,static_cast<BYTE>(combine),static_cast<BYTE>(primary),static_cast<BYTE>(secondary)};
    for (int i=0;i<4;++i) packet[7+i]=(pending>>(i*8))&255;
    SocketClient->Send(packet,sizeof(packet)); return true;
}
void Update() {
    if (!Hero || SceneFlag!=MAIN_SCENE || owner!=Hero->ID) { if (pending || state.valid) Clear(); return; }
    if (pending && SDL_GetTicks()-sentAt>=TimeoutMs) { Clear(); state.status="No server response. Reopen and select the items again; check your inventory before retrying."; }
}
void Receive(std::span<const std::uint8_t> packet) {
    if (!pending || packet.size()<5 || packet[0]!=0xC2 || packet[3]!=0xFD || packet[4]!=3 || ((packet[1]<<8)|packet[2])!=packet.size() || !Hero || owner!=Hero->ID || SceneFlag!=MAIN_SCENE) return;
    const auto json=nlohmann::json::parse(packet.begin()+5,packet.end(),nullptr,false);
    if (!json.is_object() || !json.contains("RequestId") || !json["RequestId"].is_number_unsigned() || json["RequestId"]!=pending
        || !json.contains("Valid") || !json["Valid"].is_boolean() || !json.contains("Success") || !json["Success"].is_boolean()
        || !json.contains("Cost") || !json["Cost"].is_number_integer() || json["Cost"].get<std::int64_t>()<0 || json["Cost"].get<std::int64_t>()>INT_MAX
        || !json.contains("ExcellentFlags") || !json["ExcellentFlags"].is_number_integer() || json["ExcellentFlags"].get<std::int64_t>()<0 || json["ExcellentFlags"].get<std::int64_t>()>63
        || !json.contains("Status") || !json["Status"].is_string() || json["Status"].get_ref<const std::string&>().size()>512) return;
    pending=0; state.pending=false; state.valid=json["Valid"]; state.success=json["Success"]; state.cost=json["Cost"]; state.excellentFlags=json["ExcellentFlags"]; state.status=json["Status"]; ++state.revision;
}
}

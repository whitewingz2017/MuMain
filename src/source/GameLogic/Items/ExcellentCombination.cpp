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
bool CanStackCores(const ITEM* source, const ITEM* target) {
    return source && target && source->Type==ResonanceCoreType && target->Type==ResonanceCoreType
        && source->Level==0 && target->Level==0 && source->Durability>0 && source->Durability<MaximumCoreStack
        && target->Durability>0 && target->Durability<MaximumCoreStack;
}
void Clear() { pending=0; state.pending=false; state.valid=false; state.success=false; state.completed=false; state.status.clear(); ++state.revision; }
namespace {
bool Send(int operation, int primary, int secondary, int cores) {
    if (state.pending || !SocketClient || !SocketClient->IsConnected() || SceneFlag!=MAIN_SCENE || !Hero
        || primary<0 || primary>255 || secondary<0 || secondary>255 || cores<0 || cores>100) return false;
    if (operation==1 && (!state.valid || cores!=state.selectedCores)) return false;
    pending=++sequence; if (!pending) pending=++sequence;
    owner=Hero->ID; sentAt=SDL_GetTicks();
    state.pending=true; state.valid=false; state.success=false; state.completed=false;
    state.status=operation==1 ? "Combining..." : "Checking items and Resonance Cores..."; ++state.revision;
    BYTE packet[12]={0xC1,12,0xFD,3,static_cast<BYTE>(operation),static_cast<BYTE>(primary),static_cast<BYTE>(secondary)};
    for (int i=0;i<4;++i) packet[7+i]=(pending>>(i*8))&255;
    packet[11]=static_cast<BYTE>(cores);
    SocketClient->Send(packet,sizeof(packet)); return true;
}
}
bool Request(bool combine, int primary, int secondary, int cores) { return Send(combine ? 1 : 0,primary,secondary,cores); }
bool RequestConfiguration() { state.configured=false; return Send(2,0,0,0); }
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
    for (const char* key : {"BaseRate","BonusPerCore","MaximumRate","AvailableCores","SelectedCores"}) {
        if (!json.contains(key) || !json[key].is_number_integer() || json[key].get<std::int64_t>()<0
            || json[key].get<std::int64_t>()>INT_MAX) return;
    }
    const int base=json["BaseRate"], boost=json["BonusPerCore"], cap=json["MaximumRate"], selected=json["SelectedCores"];
    if (base>100 || cap<base || cap>100 || boost<1 || boost>100 || selected>(cap-base+boost-1)/boost
        || !json.contains("Completed") || !json["Completed"].is_boolean()
        || (json["Success"].get<bool>() && !json["Completed"].get<bool>())
        || (json["Valid"].get<bool>() && (json["Completed"].get<bool>() || selected>json["AvailableCores"].get<int>()))) return;
    state.baseRate=base; state.bonusPerCore=boost; state.maximumRate=cap;
    state.availableCores=json["AvailableCores"]; state.selectedCores=selected;
    state.completed=json["Completed"]; state.configured=true;
    pending=0; state.pending=false; state.valid=json["Valid"]; state.success=json["Success"]; state.cost=json["Cost"]; state.excellentFlags=json["ExcellentFlags"]; state.status=json["Status"]; ++state.revision;
}
}

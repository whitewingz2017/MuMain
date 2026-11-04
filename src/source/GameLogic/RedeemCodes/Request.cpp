#include "stdafx.h"
#include "GameLogic/RedeemCodes/Request.h"
#include "Dotnet/Connection.h"
#include "Network/Server/WSclient.h"
#include "Scenes/SceneCommon.h"
#include <array>

namespace GameLogic::RedeemCodes
{
bool Send(std::wstring_view code, std::wstring& error)
{
    constexpr int CodeLength = 32;
    if (!SocketClient || !SocketClient->IsConnected() || SceneFlag != MAIN_SCENE)
    {
        error = L"Enter the game before redeeming a code.";
        return false;
    }
    if (code.size() < 4 || code.size() > CodeLength)
    {
        error = L"Enter a code with 4 to 32 letters, digits or hyphens.";
        return false;
    }
    std::array<BYTE, 4 + CodeLength> packet{0xC1, 4 + CodeLength, 0xFC, 0x01};
    for (size_t i = 0; i < code.size(); ++i)
    {
        const auto c = code[i];
        if (!((c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') || (c >= L'0' && c <= L'9') || c == L'-'))
        {
            error = L"Use only letters, digits or hyphens.";
            return false;
        }
        packet[4 + i] = static_cast<BYTE>(c);
    }
    SocketClient->Send(packet.data(), static_cast<int>(packet.size()));
    return true;
}
}

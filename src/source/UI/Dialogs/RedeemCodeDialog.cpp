#include "stdafx.h"
#include "UI/Dialogs/RedeemCodeDialog.h"
#include "UI/Dialogs/GenericConfirmDialog.h"
#include "UI/Core/WindowSystem.h"
#include "UI/Chat/ChatMessages.h"
#include "GameLogic/RedeemCodes/Request.h"

namespace UI::RedeemCodes
{
namespace
{
void Submit()
{
    using namespace mu::ui::window;
    std::wstring error;
    if (!GameLogic::RedeemCodes::Send(g_pGenericConfirmDialog->GetInputText(), error))
    {
        g_pGenericConfirmDialog->KeepOpen();
        UI::Chat::PostSystem(error, TYPE_ERROR_MESSAGE);
        return;
    }
    UI::Chat::PostSystem(L"Redeem request sent. Waiting for the server result.", TYPE_SYSTEM_MESSAGE);
}
}
void Open()
{
    using namespace mu::ui::window;
    if (!g_pGenericConfirmDialog) return;
    GenericDialogConfig config;
    config.title = L"Redeem Code";
    config.lines = {{L"Enter your gift code to claim its rewards."}};
    config.primaryLabel = L"Redeem";
    config.showCancel = true;
    config.input = GenericDialogConfig::InputField{};
    config.input->maxLength = 32;
    config.onPrimary = Submit;
    g_pGenericConfirmDialog->Show(std::move(config));
}
}

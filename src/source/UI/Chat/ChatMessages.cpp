#include "stdafx.h"

#include "UI/Chat/ChatMessages.h"

#include "UI/Core/WindowSystem.h"

namespace UI::Chat
{
void PostSystem(std::wstring_view text, mu::ui::window::MESSAGE_TYPE kind)
{
    g_pSystemLogBox->AddText(std::wstring(text), kind);
    // Mobile mirrors inside CSystemLogWindow::AddText.
    if (g_pChatListBox && kind == mu::ui::window::TYPE_SYSTEM_MESSAGE)
        g_pChatListBox->AddText(L"", std::wstring(text), kind);
}

void PostChat(std::wstring_view sender, std::wstring_view text,
              mu::ui::window::MESSAGE_TYPE kind, mu::ui::window::MESSAGE_TYPE errorKind)
{
    g_pChatListBox->AddText(std::wstring(sender), std::wstring(text), kind, errorKind);
}

bool IsWhisperBlocked()
{
    return g_pChatInputBox->IsBlockWhisper();
}

void ResetLogFilter()
{
    g_pChatListBox->ResetFilter();
}
}

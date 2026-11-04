#pragma once

#include "UI/Chat/MessageType.h"
#include <string_view>

namespace UI::Chat
{
struct MessagePresentation
{
    std::wstring_view sender;
    std::wstring_view text;
    std::wstring_view prefix;
    const char* kind;
};

const char* MessageKind(mu::ui::window::MESSAGE_TYPE type);
MessagePresentation PresentMessage(std::wstring_view sender, std::wstring_view text,
                                   mu::ui::window::MESSAGE_TYPE type);
} // namespace UI::Chat

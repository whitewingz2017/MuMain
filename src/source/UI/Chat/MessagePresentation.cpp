#include "UI/Chat/MessagePresentation.h"

namespace UI::Chat
{
const char* MessageKind(mu::ui::window::MESSAGE_TYPE type)
{
    using namespace mu::ui::window;
    switch (type)
    {
    case TYPE_CHAT_MESSAGE: return "chat";
    case TYPE_WHISPER_MESSAGE: return "whisper";
    case TYPE_SYSTEM_MESSAGE: return "system";
    case TYPE_ERROR_MESSAGE: return "error";
    case TYPE_PARTY_MESSAGE: return "party";
    case TYPE_GUILD_MESSAGE: return "guild";
    case TYPE_UNION_MESSAGE: return "union";
    case TYPE_GENS_MESSAGE: return "gens";
    case TYPE_GM_MESSAGE: return "gm";
    case TYPE_LOOT_MESSAGE: return "loot";
    case TYPE_EXPERIENCE_MESSAGE: return "experience";
    default: return nullptr;
    }
}

MessagePresentation PresentMessage(std::wstring_view sender, std::wstring_view text,
                                   mu::ui::window::MESSAGE_TYPE type)
{
    using namespace mu::ui::window;
    MessagePresentation result{sender, text, {}, MessageKind(type)};
    constexpr std::wstring_view PostSender = L"[POST]";
    constexpr std::wstring_view PostPrefix = L"[POST] ";
    constexpr std::wstring_view PostWithColon = L"[POST]: ";
    constexpr std::size_t MaximumSenderLength = 10;
    if (type != TYPE_CHAT_MESSAGE && type != TYPE_SYSTEM_MESSAGE)
        return result;

    const bool senderIsPost = sender == PostSender || sender == L"[POST]:";
    auto posted = text;
    if (!senderIsPost)
    {
        if (text.starts_with(PostPrefix))
            posted.remove_prefix(PostPrefix.size());
        else if (text.starts_with(PostWithColon))
            posted.remove_prefix(PostWithColon.size());
        else
            return result;
    }
    const auto separator = posted.find(L": ");
    if (separator == 0 || separator > MaximumSenderLength)
        return result;
    if (!sender.empty() && !senderIsPost && sender != posted.substr(0, separator))
        return result;

    result.kind = "world";
    result.prefix = L"[POST]";
    result.sender = posted.substr(0, separator);
    result.text = posted.substr(separator + 2);
    return result;
}
} // namespace UI::Chat

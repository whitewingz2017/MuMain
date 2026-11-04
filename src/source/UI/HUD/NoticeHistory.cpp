#include "UI/HUD/NoticeHistory.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace UI::Notices
{
bool NoticeHistory::HasMessages() const
{
    return std::any_of(m_Entries.begin(), m_Entries.end(),
                       [](const Entry& entry) { return !entry.text.empty(); });
}

void NoticeHistory::Push(std::wstring_view text, bool guild)
{
    Entry incoming{std::wstring(text.substr(0, MaximumTextLength)), guild};
    m_Outgoing = std::move(m_Entries.back());
    for (std::size_t index = Capacity - 1; index > 0; --index)
        m_Entries[index] = std::move(m_Entries[index - 1]);
    m_Entries.front() = std::move(incoming);
    m_IdleAge = 0.0;
    m_OutgoingAge = 0.0;
    ++m_Revision;
}

bool NoticeHistory::Advance(double seconds)
{
    if (!std::isfinite(seconds) || seconds <= 0.0)
        return false;
    bool changed = false;
    if (!m_Outgoing.text.empty())
    {
        m_OutgoingAge += seconds;
        if (m_OutgoingAge >= ExitSeconds)
        {
            m_Outgoing = {};
            changed = true;
        }
    }
    m_IdleAge += seconds;
    if (m_IdleAge >= IdleSeconds && HasMessages())
    {
        Push({}, false);
        return true;
    }
    return changed;
}

void NoticeHistory::Clear()
{
    m_Entries = {};
    m_Outgoing = {};
    m_IdleAge = 0.0;
    m_OutgoingAge = 0.0;
    ++m_Revision;
}
} // namespace UI::Notices

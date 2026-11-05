#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace UI::Notices
{
// The five retained messages and the last message leaving the stack. Appearance belongs to RCSS.
class NoticeHistory
{
public:
    static constexpr std::size_t Capacity = 5;
    static constexpr std::size_t MaximumTextLength = 255;
    static constexpr double IdleSeconds = 15.0;
    static constexpr double ExitSeconds = 1.4;

    struct Entry
    {
        std::wstring text;
        bool guild = false;
    };

    const std::array<Entry, Capacity>& Entries() const { return m_Entries; }
    const Entry& Outgoing() const { return m_Outgoing; }
    std::uint64_t Revision() const { return m_Revision; }

    bool HasMessages() const;
    void Push(std::wstring_view text, bool guild);
    bool Advance(double seconds);
    void Clear();

private:
    std::array<Entry, Capacity> m_Entries;
    Entry m_Outgoing;
    double m_IdleAge = 0.0;
    double m_OutgoingAge = 0.0;
    std::uint64_t m_Revision = 0;
};
} // namespace UI::Notices

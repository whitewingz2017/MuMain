#pragma once

#include "UI/HUD/NoticeHistory.h"
#include <RmlUi/Core/Types.h>

#include <vector>

namespace UI::Notices
{
// Stable rows let RCSS transition from each message's previous position in the history.
struct NoticeLineEntry
{
    Rml::String text;
    Rml::String kind;
    Rml::String stage = "empty";

    bool operator==(const NoticeLineEntry& other) const = default;
};

struct NoticesRmlModel
{
    std::vector<NoticeLineEntry> lines = std::vector<NoticeLineEntry>(NoticeHistory::Capacity);
    Rml::String outgoingText;
    Rml::String outgoingKind;
    Rml::String outgoingStage = "empty";
};
} // namespace UI::Notices

#pragma once

namespace UI::Character
{
constexpr int MonsterTargetKey(int helperKey, int selectedKey, int attackedKey, int previousKey)
{
    if (helperKey >= 0) return helperKey;
    if (selectedKey >= 0) return selectedKey;
    if (attackedKey >= 0) return attackedKey;
    return previousKey;
}

constexpr bool HasVisibleLabel(bool nameVisible, float nameTime, float firstChatTime, float secondChatTime)
{
    return (nameVisible && nameTime > 0) || firstChatTime > 0 || secondChatTime > 0;
}
}

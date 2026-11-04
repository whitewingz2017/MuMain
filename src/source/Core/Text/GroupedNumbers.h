#pragma once
#include <string>
#include <string_view>

namespace Core::Text
{
// Group integer runs without depending on the device locale or changing surrounding translated text.
inline std::wstring GroupedNumbers(std::wstring_view text)
{
    std::wstring result;
    result.reserve(text.size());
    for (size_t i = 0; i < text.size();)
    {
        if (text[i] < L'0' || text[i] > L'9') { result += text[i++]; continue; }
        size_t end = i;
        while (end < text.size() && text[end] >= L'0' && text[end] <= L'9') ++end;
        for (size_t digit = i; digit < end; ++digit)
        {
            if (digit != i && (end - digit) % 3 == 0) result += L',';
            result += text[digit];
        }
        i = end;
    }
    return result;
}
}

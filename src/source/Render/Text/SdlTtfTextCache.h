#pragma once

#include <SDL3_ttf/SDL_ttf.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace Render::Text
{
class SdlTtfTextCache final
{
public:
    using TextOwner = std::unique_ptr<TTF_Text, decltype(&TTF_DestroyText)>;

    struct PreparedText
    {
        TTF_Text* text = nullptr;
        int width = 0;
        int height = 0;
        TextOwner transient{nullptr, TTF_DestroyText};
    };

    // Retain recently used labels up to the configured capacity.
    explicit SdlTtfTextCache(TTF_TextEngine* engine = nullptr, std::size_t capacity = 256);
    ~SdlTtfTextCache();

    PreparedText Prepare(TTF_Font* font, std::string_view utf8);
    void Reset(TTF_TextEngine* engine);
    void Clear();
    [[nodiscard]] std::size_t Size() const;

private:
    struct Entry
    {
        TTF_Font* font = nullptr;
        std::string utf8;
        TextOwner text{nullptr, TTF_DestroyText};
        int width = 0;
        int height = 0;
        std::uint64_t lastUsed = 0;
    };

    PreparedText Retain(TTF_Font* font, std::string_view utf8, TextOwner text, int width, int height);

    TTF_TextEngine* m_engine = nullptr;
    std::size_t m_capacity = 0;
    std::vector<Entry> m_entries;
    std::uint64_t m_useCounter = 0;
};
}

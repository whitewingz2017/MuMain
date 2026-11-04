#include "SdlTtfTextCache.h"

#include <algorithm>
#include <new>
#include <utility>

namespace Render::Text
{
SdlTtfTextCache::SdlTtfTextCache(TTF_TextEngine* engine, std::size_t capacity)
    : m_engine(engine), m_capacity(capacity)
{
}

SdlTtfTextCache::~SdlTtfTextCache() = default;

SdlTtfTextCache::PreparedText SdlTtfTextCache::Prepare(TTF_Font* font, std::string_view utf8)
{
    if (m_engine == nullptr || font == nullptr)
        return {};

    for (Entry& entry : m_entries)
    {
        if (entry.font == font && entry.utf8 == utf8)
        {
            entry.lastUsed = ++m_useCounter;
            return {entry.text.get(), entry.width, entry.height};
        }
    }

    TextOwner text(TTF_CreateText(m_engine, font, utf8.data(), utf8.size()), TTF_DestroyText);
    if (text == nullptr)
        return {};

    int width = 0;
    int height = 0;
    if (!TTF_GetTextSize(text.get(), &width, &height))
        return {};

    return Retain(font, utf8, std::move(text), width, height);
}

SdlTtfTextCache::PreparedText SdlTtfTextCache::Retain(TTF_Font* font, std::string_view utf8, TextOwner text,
                                                   int width, int height)
{
    PreparedText prepared{text.get(), width, height};
    if (m_capacity == 0)
    {
        prepared.transient = std::move(text);
        return prepared;
    }

    try
    {
        std::string key(utf8);
        if (m_entries.capacity() < m_capacity)
            m_entries.reserve(m_capacity);
        Entry entry{font, std::move(key), std::move(text), width, height, ++m_useCounter};
        if (m_entries.size() < m_capacity)
        {
            m_entries.push_back(std::move(entry));
            return prepared;
        }

        auto oldest = std::min_element(m_entries.begin(), m_entries.end(), [](const Entry& a, const Entry& b)
        {
            return a.lastUsed < b.lastUsed;
        });
        *oldest = std::move(entry);
        return prepared;
    }
    catch (const std::bad_alloc&)
    {
        prepared.transient = std::move(text);
        return prepared;
    }
}

void SdlTtfTextCache::Reset(TTF_TextEngine* engine)
{
    Clear();
    m_engine = engine;
}

void SdlTtfTextCache::Clear()
{
    m_entries.clear();
    m_useCounter = 0;
}

std::size_t SdlTtfTextCache::Size() const
{
    return m_entries.size();
}
}

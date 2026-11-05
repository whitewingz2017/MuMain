#pragma once

#include <optional>

namespace Render::Models::CosmeticArmor
{
// Restore only a part actually replaced for drawing; untouched animation state must advance.
template <typename Part> class ScopedPartOverride
{
public:
    explicit ScopedPartOverride(Part& part) : m_part(part) {}
    ScopedPartOverride(const ScopedPartOverride&) = delete;
    ScopedPartOverride& operator=(const ScopedPartOverride&) = delete;
    ~ScopedPartOverride()
    {
        if (m_original) m_part = *m_original;
    }
    void Save()
    {
        if (!m_original) m_original = m_part;
    }

private:
    Part& m_part;
    std::optional<Part> m_original;
};
}

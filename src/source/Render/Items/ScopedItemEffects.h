#pragma once

#include <utility>

namespace Render::Items
{
class ScopedItemEffects
{
public:
    explicit ScopedItemEffects(bool enabled, bool localParticles = false)
        : m_previous(std::exchange(s_enabled, enabled)),
          m_previousLocalParticles(std::exchange(s_localParticles, localParticles)) {}
    ~ScopedItemEffects() { s_enabled = m_previous; s_localParticles = m_previousLocalParticles; }
    ScopedItemEffects(const ScopedItemEffects&) = delete;
    ScopedItemEffects& operator=(const ScopedItemEffects&) = delete;
    static bool Enabled() { return s_enabled; }
    static bool LocalParticles() { return s_localParticles; }

private:
    inline static bool s_enabled = true;
    inline static bool s_localParticles = false;
    bool m_previous;
    bool m_previousLocalParticles;
};
}

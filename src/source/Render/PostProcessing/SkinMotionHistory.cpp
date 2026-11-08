#include "Render/PostProcessing/SkinMotionHistory.h"

#include <algorithm>
#include <iterator>
#include <utility>

namespace Render::PostProcessing
{

namespace
{
constexpr std::size_t MaxPlacements = 8u;
} // namespace

std::optional<PreviousSkin> SkinMotionHistory::Record(const void* key, std::uint32_t version,
                                                      std::span<const float> bones, const SkinPlacement& placement)
{
    const auto [current, inserted] = m_current.try_emplace(key);
    Entry& entry = current->second;
    if (inserted)
    {
        entry.bones.assign(bones.begin(), bones.end());
        entry.version = version;
    }
    else if (!entry.shared && entry.version != version)
    {
        // Each Transform() of the same pose bumps the version: only other bones make the palette shared.
        entry.shared = !MatchesPose(entry, bones);
        entry.version = version;
    }
    else if (!entry.shared && bones.size() > entry.bones.size())
    {
        entry.shared = !MatchesPose(entry, bones);
    }
    if (entry.shared)
    {
        return std::nullopt;
    }
    const std::size_t turn = PlacementTurn(entry, placement);
    // More placements than an object draws itself at: the bones place many objects.
    if (turn >= MaxPlacements)
    {
        entry.shared = true;
        return std::nullopt;
    }

    const auto previous = m_previous.find(key);
    if (previous == m_previous.end())
    {
        return std::nullopt;
    }
    const Entry& before = previous->second;
    if (before.shared || before.bones.size() < bones.size() || turn >= before.placements.size())
    {
        return std::nullopt;
    }
    return PreviousSkin{before.bones, before.placements[turn]};
}

bool SkinMotionHistory::MatchesPose(Entry& entry, std::span<const float> bones)
{
    std::vector<float>& kept = entry.bones;
    const auto common = static_cast<std::ptrdiff_t>(std::min(kept.size(), bones.size()));
    if (!std::equal(bones.begin(), bones.begin() + common, kept.begin()))
    {
        return false;
    }
    kept.insert(kept.end(), bones.begin() + common, bones.end());
    return true;
}

std::size_t SkinMotionHistory::PlacementTurn(Entry& entry, const SkinPlacement& placement)
{
    const auto found = std::find(entry.placements.begin(), entry.placements.end(), placement);
    if (found != entry.placements.end())
    {
        return static_cast<std::size_t>(std::distance(entry.placements.begin(), found));
    }
    entry.placements.push_back(placement);
    return entry.placements.size() - 1u;
}

void SkinMotionHistory::BeginFrame()
{
    std::swap(m_previous, m_current);
    m_current.clear();
}

void SkinMotionHistory::Reset()
{
    m_current.clear();
    m_previous.clear();
}

} // namespace Render::PostProcessing

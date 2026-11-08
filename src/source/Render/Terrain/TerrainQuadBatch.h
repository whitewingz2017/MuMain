#pragma once

#include "Render/Renderer/MuRenderer.h"
#include "Render/Renderer/QuadTopology.h"

#include <algorithm>
#include <map>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace Render
{
// Opaque, non-overlapping ground quads between order-sensitive terrain draws.
class TerrainQuadBatch
{
public:
    void Add(std::uint32_t texture, std::span<const mu::Vertex3D, 4> vertices);

    // Returns the last recorded texture so the caller can restore its binding.
    template <typename Draw> std::optional<std::uint32_t> Flush(Draw&& draw)
    {
        if (!m_lastTexture)
            return std::nullopt;

        constexpr std::size_t MaxVertices = Topology::MaxQuadsPerDraw * 4u;
        for (auto& [texture, vertices] : m_vertices)
        {
            auto remaining = std::span<const mu::Vertex3D>(vertices);
            while (!remaining.empty())
            {
                const std::size_t count = std::min(remaining.size(), MaxVertices);
                draw(texture, remaining.first(count));
                remaining = remaining.subspan(count);
            }
            // Keep the allocation for the next frame.
            vertices.clear();
        }
        return std::exchange(m_lastTexture, std::nullopt);
    }

private:
    std::map<std::uint32_t, std::vector<mu::Vertex3D>> m_vertices;
    std::optional<std::uint32_t> m_lastTexture;
};
} // namespace Render

#include "TerrainQuadBatch.h"

namespace Render
{
void TerrainQuadBatch::Add(std::uint32_t texture, std::span<const mu::Vertex3D, 4> vertices)
{
    auto& stored = m_vertices[texture];
    stored.insert(stored.end(), vertices.begin(), vertices.end());
    m_lastTexture = texture;
}
} // namespace Render

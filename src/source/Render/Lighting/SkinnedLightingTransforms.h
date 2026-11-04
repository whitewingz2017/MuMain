#pragma once

#include <glm/glm.hpp>

namespace Render::Lighting
{
class SkinnedLightingTransforms final
{
public:
    // True when a changed camera or model-view required new lighting transforms.
    bool Update(const glm::mat4& modelView, const glm::mat4& worldFromView);
    const glm::vec4& EyePosition() const { return m_eyePosition; }
    const glm::mat4& VertexToWorld() const { return m_vertexToWorld; }

private:
    glm::mat4 m_modelView{1.0f};
    glm::mat4 m_worldFromView{1.0f};
    glm::vec4 m_eyePosition{};
    glm::mat4 m_vertexToWorld{1.0f};
    bool m_valid = false;
};
} // namespace Render::Lighting

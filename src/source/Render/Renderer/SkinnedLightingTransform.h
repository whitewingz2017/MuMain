#pragma once

#include <glm/glm.hpp>

namespace Render
{
// Enhanced lighting depends on the draw's view and the scene's view, shared across meshes.
class SkinnedLightingTransform
{
public:
    struct Uniforms
    {
        glm::vec4 eyePosition{0.0f};
        float vertexToWorldRows[3][4]{};
    };

    [[nodiscard]] const Uniforms& Get(const glm::mat4& modelView, const glm::mat4& worldFromView);

private:
    glm::mat4 m_modelView{1.0f};
    glm::mat4 m_worldFromView{1.0f};
    Uniforms m_uniforms;
    bool m_valid = false;
};
} // namespace Render

#include "SkinnedLightingTransform.h"

#include <glm/gtc/type_ptr.hpp>

#include <cstring>

namespace Render
{
const SkinnedLightingTransform::Uniforms& SkinnedLightingTransform::Get(const glm::mat4& modelView,
                                                                        const glm::mat4& worldFromView)
{
    if (m_valid && std::memcmp(glm::value_ptr(modelView), glm::value_ptr(m_modelView), sizeof(modelView)) == 0 &&
        std::memcmp(glm::value_ptr(worldFromView), glm::value_ptr(m_worldFromView), sizeof(worldFromView)) == 0)
    {
        return m_uniforms;
    }

    m_modelView = modelView;
    m_worldFromView = worldFromView;
    m_uniforms.eyePosition = glm::inverse(modelView) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    const glm::mat4 vertexToWorld = worldFromView * modelView;
    for (int row = 0; row < 3; ++row)
    {
        for (int column = 0; column < 4; ++column)
            m_uniforms.vertexToWorldRows[row][column] = vertexToWorld[column][row];
    }
    m_valid = true;
    return m_uniforms;
}
} // namespace Render

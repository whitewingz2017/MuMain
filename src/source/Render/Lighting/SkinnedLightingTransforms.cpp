#include "SkinnedLightingTransforms.h"

namespace Render::Lighting
{
bool SkinnedLightingTransforms::Update(const glm::mat4& modelView, const glm::mat4& worldFromView)
{
    if (m_valid && m_modelView == modelView && m_worldFromView == worldFromView)
    {
        return false;
    }

    m_eyePosition = glm::inverse(modelView) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    m_vertexToWorld = worldFromView * modelView;
    m_modelView = modelView;
    m_worldFromView = worldFromView;
    m_valid = true;
    return true;
}
} // namespace Render::Lighting

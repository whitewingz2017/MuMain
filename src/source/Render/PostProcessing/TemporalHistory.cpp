#include "Render/PostProcessing/TemporalHistory.h"

#include "Render/PostProcessing/TemporalJitter.h"

namespace Render::PostProcessing
{

TemporalFrame TemporalHistory::BeginFrame(const glm::mat4& camera, std::uint32_t width, std::uint32_t height)
{
    TemporalFrame frame;
    frame.jitterClip = TemporalJitterClip(TemporalJitterPixels(m_frameIndex), width, height);
    m_frameIndex = (m_frameIndex + 1u) % TemporalJitterFrameCount;

    const bool sameSize = m_width == width && m_height == height;
    if (m_previousCamera && sameSize)
    {
        if (const std::optional<glm::mat4> reprojection = CurrentToPreviousClip(*m_previousCamera, camera))
        {
            frame.currentToPrevious = *reprojection;
            frame.historyWeight = TemporalHistoryWeight;
        }
    }

    m_previousCamera = camera;
    m_width = width;
    m_height = height;
    return frame;
}

void TemporalHistory::Reset()
{
    m_previousCamera.reset();
}

} // namespace Render::PostProcessing

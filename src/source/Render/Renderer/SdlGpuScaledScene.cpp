#include "SdlGpuScaledScene.h"

#include <algorithm>
#include <cmath>

namespace mu
{

SDL_GPUViewport ScaleSceneViewport(SDL_GPUViewport viewport, float scaleX, float scaleY)
{
    viewport.x *= scaleX;
    viewport.y *= scaleY;
    viewport.w *= scaleX;
    viewport.h *= scaleY;
    return viewport;
}

SDL_Rect ScaleSceneScissor(SDL_Rect scissor, float scaleX, float scaleY)
{
    if (scaleX == 1.f && scaleY == 1.f)
        return scissor;
    const int right = static_cast<int>(std::ceil((scissor.x + scissor.w) * scaleX));
    const int bottom = static_cast<int>(std::ceil((scissor.y + scissor.h) * scaleY));
    scissor.x = static_cast<int>(std::floor(scissor.x * scaleX));
    scissor.y = static_cast<int>(std::floor(scissor.y * scaleY));
    scissor.w = right - scissor.x;
    scissor.h = bottom - scissor.y;
    return scissor;
}

bool SdlGpuScaledScene::Ensure(SDL_GPUDevice* device, SDL_GPUTextureFormat format,
                               SdlGpuTargetSize fullSize, float scale)
{
    if (!device || !fullSize.width || !fullSize.height || !std::isfinite(scale) || scale <= 0.f || scale >= 1.f)
        return false;
    const SdlGpuTargetSize size{
        std::max(1u, static_cast<Uint32>(std::lround(fullSize.width * scale))),
        std::max(1u, static_cast<Uint32>(std::lround(fullSize.height * scale)))};
    if (m_device == device && m_format == format &&
        m_size.width == size.width && m_size.height == size.height)
        return m_color && m_depth;

    const auto colorInfo = GetSdlGpuFrameCaptureTextureInfo(format, size.width, size.height);
    const auto depthInfo = GetSdlGpuDepthTextureInfo(size, SDL_GPU_SAMPLECOUNT_1);
    if (!colorInfo || !depthInfo)
        return false;
    Release();
    // Remember failed allocations too, so a low-memory phone doesn't retry every frame.
    m_device = device;
    m_format = format;
    m_size = size;
    SDL_GPUTexture* color = SDL_CreateGPUTexture(device, &*colorInfo);
    SDL_GPUTexture* depth = color ? SDL_CreateGPUTexture(device, &*depthInfo) : nullptr;
    if (!color || !depth)
    {
        if (color) SDL_ReleaseGPUTexture(device, color);
        return false;
    }
    m_color = color;
    m_depth = depth;
    return true;
}

void SdlGpuScaledScene::Release()
{
    if (m_color) SDL_ReleaseGPUTexture(m_device, m_color);
    if (m_depth) SDL_ReleaseGPUTexture(m_device, m_depth);
    m_device = nullptr;
    m_color = m_depth = nullptr;
    m_format = SDL_GPU_TEXTUREFORMAT_INVALID;
    m_size = {};
}

} // namespace mu

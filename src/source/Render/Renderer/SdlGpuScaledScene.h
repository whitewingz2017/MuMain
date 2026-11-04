#pragma once

#include "SdlGpuPixelFormat.h"

namespace mu
{

[[nodiscard]] SDL_GPUViewport ScaleSceneViewport(SDL_GPUViewport viewport, float scaleX, float scaleY);
[[nodiscard]] SDL_Rect ScaleSceneScissor(SDL_Rect scissor, float scaleX, float scaleY);

// A smaller world target; the finished world is enlarged before full-resolution UI is drawn.
class SdlGpuScaledScene
{
public:
    SdlGpuScaledScene() = default;
    ~SdlGpuScaledScene() { Release(); }
    SdlGpuScaledScene(const SdlGpuScaledScene&) = delete;
    SdlGpuScaledScene& operator=(const SdlGpuScaledScene&) = delete;
    [[nodiscard]] bool Ensure(SDL_GPUDevice* device, SDL_GPUTextureFormat format,
                              SdlGpuTargetSize fullSize, float scale);
    void Release();
    [[nodiscard]] SDL_GPUTexture* Color() const { return m_color; }
    [[nodiscard]] SDL_GPUTexture* Depth() const { return m_depth; }
    [[nodiscard]] SdlGpuTargetSize Size() const { return m_size; }

private:
    SDL_GPUDevice* m_device = nullptr;
    SDL_GPUTexture* m_color = nullptr;
    SDL_GPUTexture* m_depth = nullptr;
    SDL_GPUTextureFormat m_format = SDL_GPU_TEXTUREFORMAT_INVALID;
    SdlGpuTargetSize m_size{};
};

} // namespace mu

#pragma once

#include "SdlGpuPixelFormat.h"
#include "MuRenderer.h"
#include "Render/PostProcessing/ColorGradingTable.h"
#include "Render/PostProcessing/TemporalHistory.h"

#include <SDL3/SDL_gpu.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace mu
{

// Shaders of the post-processing passes. The caller owns them; they are needed
// only while the pipelines are built.
struct SdlGpuPostShaders
{
    SDL_GPUShader* fullscreenVertex = nullptr;
    SDL_GPUShader* bloomDown = nullptr;
    SDL_GPUShader* bloomUp = nullptr;
    SDL_GPUShader* composite = nullptr;
    // Optional: without it temporal anti-aliasing is unavailable.
    SDL_GPUShader* temporal = nullptr;
};

// Mirrors BloomUniforms in post_bloom_down.frag.hlsl and post_bloom_up.frag.hlsl.
struct SdlGpuBloomUniform
{
    float sourceTexelSize[2];
    float levelWeight;
    float padding;
};
static_assert(sizeof(SdlGpuBloomUniform) == 16);

// Mirrors CompositeUniforms in post_composite.frag.hlsl.
struct SdlGpuCompositeUniform
{
    float inverseTargetSize[2];
    float bloomScale;
    // 0: ungraded; otherwise the blend of the graded colour with the ungraded one
    // (Render::PostProcessing::CompositeGradingStrength).
    float gradingStrength;
    // 0: unsharpened; up to 1 (Render::PostProcessing::CompositeSharpenStrength).
    float sharpenStrength;
    float padding[3];
};
static_assert(offsetof(SdlGpuCompositeUniform, bloomScale) == 8);
static_assert(offsetof(SdlGpuCompositeUniform, gradingStrength) == 12);
static_assert(offsetof(SdlGpuCompositeUniform, sharpenStrength) == 16);
static_assert(sizeof(SdlGpuCompositeUniform) == 32);

// Mirrors TemporalUniforms in post_temporal.frag.hlsl.
struct SdlGpuTemporalUniform
{
    // Column-major, as glm stores it.
    float currentToPrevious[16];
    float inverseTargetSize[2];
    float jitter[2];
    float historyWeight;
    float objectMotionWritten;
    float padding[2];
};
static_assert(offsetof(SdlGpuTemporalUniform, inverseTargetSize) == 64);
static_assert(offsetof(SdlGpuTemporalUniform, jitter) == 72);
static_assert(offsetof(SdlGpuTemporalUniform, historyWeight) == 80);
static_assert(offsetof(SdlGpuTemporalUniform, objectMotionWritten) == 84);
static_assert(sizeof(SdlGpuTemporalUniform) == 96);

// The textures and passes that turn the rendered 3D scene into the processed
// image the UI is then drawn on: the scene texture the scene pass resolves into,
// the temporal anti-aliasing history and object motion, the bloom source the
// additive draws are replayed into, the bloom blur chain and the composite with
// colour grading.
class SdlGpuPostProcessing
{
public:
    static constexpr std::size_t BloomLevelCount = 5;
    // The built-in level falloff (look profile bloom.levelFalloff): each wider
    // level adds this share of itself to the next narrower one, so a level's
    // weight in the final glow falls with its radius (1, 0.5, 0.25, ...): the
    // glow stays near its effect instead of veiling the frame.
    static constexpr float BloomLevelWeight = 0.5f;

    // Builds the pipelines for frames of `frameFormat` with `sampleCount`
    // samples per pixel. The composite draws inside the frame's UI pass, which
    // has a depth target. False when any is missing: post-processing is then
    // unavailable.
    [[nodiscard]] bool CreatePipelines(SDL_GPUDevice* device, const SdlGpuPostShaders& shaders,
                                       SDL_GPUTextureFormat frameFormat, SDL_GPUSampleCount sampleCount);
    void DestroyPipelines();
    [[nodiscard]] bool IsAvailable() const;

    // The scene, bloom source and bloom chain textures for frames of `size`,
    // recreated when the size changed.
    [[nodiscard]] bool EnsureTargets(SdlGpuTargetSize size);
    [[nodiscard]] SDL_GPUTexture* GetSceneTexture() const;
    [[nodiscard]] SDL_GPUTexture* GetBloomSourceTexture() const;

    // Whether ResolveTemporal can run: its pipeline was built.
    [[nodiscard]] bool IsTemporalAvailable() const;

    // The history textures and the object motion texture, at the size of the
    // last EnsureTargets; made at the first call. False when they cannot be made.
    [[nodiscard]] bool EnsureTemporalTargets();

    // Where the motion of this frame's skinned draws is drawn (object_motion.frag),
    // in its format; null (and INVALID) without a float format to hold it, or
    // before EnsureTemporalTargets.
    [[nodiscard]] SDL_GPUTexture* GetObjectMotionTexture() const;
    [[nodiscard]] SDL_GPUTextureFormat GetObjectMotionFormat() const;

    // Blends this frame's scene, drawn at `frame`'s jitter over the depth
    // `sceneDepth`, with the previous frames' result into the next history; the
    // composite of the same frame shows that history as the scene.
    // `objectMotionWritten`: the object motion texture holds this frame's skinned
    // draws. False (the composite shows the scene itself) when the history cannot
    // be made.
    [[nodiscard]] bool ResolveTemporal(SDL_GPUCommandBuffer* commandBuffer, SDL_GPUTexture* sceneDepth,
                                       bool objectMotionWritten, const Render::PostProcessing::TemporalFrame& frame);

    // Blurs the bloom source down and back up the chain, each level added to the
    // next larger one at `levelFalloff`; the composite of the same frame adds the result.
    void RenderBloom(SDL_GPUCommandBuffer* commandBuffer, float levelFalloff);

    // Draws the processed scene over the whole target of `pass`: the scene (or
    // its temporal resolve, when ResolveTemporal ran this frame), sharpened at
    // the look's sharpening strength,
    // plus this frame's bloom scaled by the look's bloom strength when
    // RenderBloom ran (with the same level falloff), graded through the grading
    // table at the look's grading strength when one is set. The diagnostic
    // views show the bloom source or the bloom instead, ungraded.
    void DrawComposite(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* pass, SdlGpuTargetSize size,
                       const PostProcessingLook& look, PostProcessingView view);

    // Replaces the grading table (Render::PostProcessing::LoadGradingTable's
    // RGBA8 texels), or empty for none. False (and no table) when it cannot be uploaded.
    [[nodiscard]] bool SetGradingTable(std::span<const std::uint8_t> rgba);
    [[nodiscard]] bool HasGradingTable() const;

    // Everything, at shutdown.
    void Release();

private:
    struct BloomLevel
    {
        SDL_GPUTexture* texture = nullptr;
        SdlGpuTargetSize size{};
    };

    // Optional: a missing shader or a failed build leaves temporal anti-aliasing unavailable.
    void CreateTemporalPipeline(const SdlGpuPostShaders& shaders);
    [[nodiscard]] bool CreateSamplers();
    [[nodiscard]] bool CreateGradingTexture();
    void ReleaseTargets();
    void RunBloomPass(SDL_GPUCommandBuffer* commandBuffer, SDL_GPUGraphicsPipeline* pipeline, const BloomLevel& target,
                      SDL_GPULoadOp loadOp, SDL_GPUTexture* source, SdlGpuTargetSize sourceSize, float levelWeight);

    SDL_GPUDevice* m_device = nullptr;
    SDL_GPUTextureFormat m_frameFormat = SDL_GPU_TEXTUREFORMAT_INVALID;
    SDL_GPUTextureFormat m_bloomFormat = SDL_GPU_TEXTUREFORMAT_INVALID;
    SDL_GPUTextureFormat m_historyFormat = SDL_GPU_TEXTUREFORMAT_INVALID;
    SDL_GPUTextureFormat m_objectMotionFormat = SDL_GPU_TEXTUREFORMAT_INVALID;
    SDL_GPUGraphicsPipeline* m_bloomDownPipeline = nullptr;
    SDL_GPUGraphicsPipeline* m_bloomUpPipeline = nullptr;
    SDL_GPUGraphicsPipeline* m_compositePipeline = nullptr;
    SDL_GPUGraphicsPipeline* m_temporalPipeline = nullptr;
    SDL_GPUSampler* m_pointSampler = nullptr;
    SDL_GPUSampler* m_linearSampler = nullptr;

    SdlGpuTargetSize m_targetSize{};
    SDL_GPUTexture* m_sceneTexture = nullptr;
    SDL_GPUTexture* m_bloomSourceTexture = nullptr;
    std::array<BloomLevel, BloomLevelCount> m_bloomLevels{};
    bool m_bloomRendered = false;

    // Two histories in turn: each resolve reads the one the previous resolve wrote.
    // Made at the first resolve, so frames without temporal anti-aliasing do not hold them.
    std::array<SDL_GPUTexture*, 2> m_historyTextures{};
    // The history the latest resolve wrote.
    std::size_t m_historyIndex = 0u;
    // That history holds a resolved frame (not a texture made since).
    bool m_historyValid = false;
    SDL_GPUTexture* m_objectMotionTexture = nullptr;
    // ResolveTemporal ran this frame: the composite shows the history.
    bool m_temporalResolved = false;

    SDL_GPUTexture* m_gradingTexture = nullptr;
    bool m_gradingEnabled = false;
};

} // namespace mu

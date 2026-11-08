#include "stdafx.h"
#include <doctest.h>

#include "Core/Utilities/Benchmark/BenchScene.h"
#include "Data/GameConfig/GameConfig.h"
#include "Render/Renderer/MuRenderer.h"

namespace
{
class AntialiasingRenderer final : public mu::IMuRenderer
{
public:
    int samples = 1;
    bool temporal = false;
    int temporalChanges = 0;

    void RenderQuad2D(std::span<const mu::Vertex2D>, std::uint32_t) override {}
    void RenderTriangles(std::span<const mu::Vertex3D>, std::uint32_t) override {}
    void RenderQuad3D(std::span<const mu::Vertex3D>, std::uint32_t) override {}
    void RenderQuadStrip(std::span<const mu::Vertex3D>, std::uint32_t) override {}
    void RenderLines(std::span<const mu::Vertex3D>, std::uint32_t) override {}
    void SetBlendMode(mu::BlendMode) override {}
    void DisableBlend() override {}
    void SetDepthTest(bool) override {}
    void SetFog(const mu::FogParams&) override {}
    void BeginScene(int, int, int, int) override {}
    void EndScene() override {}
    void Begin2DPass() override {}
    void End2DPass() override {}
    void ClearScreen() override {}
    void SetAntialiasingSamples(int value) override
    {
        samples = value > 1 ? value : 1;
    }
    int GetAntialiasingSamples() const override
    {
        return samples;
    }
    bool IsTemporalAntialiasingRequested() const override
    {
        return temporal;
    }
    void SetTemporalAntialiasing(bool enabled) override
    {
        temporal = enabled;
        ++temporalChanges;
    }
};
} // namespace

TEST_CASE("aa.off restores the requested anti-aliasing for the next segment and the end of the run [benchmark]")
{
    using namespace Core::Benchmark;
    const AntialiasingType savedType = GameConfig::GetInstance().GetAntialiasingType();
    const int savedSamples = GameConfig::GetInstance().GetAntialiasing();
    for (bool temporal : {false, true})
    {
        CAPTURE(temporal);
        AntialiasingRenderer renderer;
        renderer.samples = temporal ? 1 : 4;
        renderer.temporal = temporal;
        mu::SetRendererOverride(&renderer);
        const SceneConfig before = Scene::CaptureCurrentConfig();
        Scene::ApplyConfig(Segments::Find("aa.off")->config);
        CHECK(renderer.samples == 1);
        CHECK_FALSE(renderer.temporal);

        Scene::ApplyConfig(Segments::Find(kBaselineSegmentName)->config);
        CHECK(renderer.samples == (temporal ? 1 : 4));
        CHECK(renderer.temporal == temporal);
        Scene::ApplyConfig(before);
        CHECK(renderer.samples == (temporal ? 1 : 4));
        CHECK(renderer.temporal == temporal);
        CHECK(GameConfig::GetInstance().GetAntialiasingType() == savedType);
        CHECK(GameConfig::GetInstance().GetAntialiasing() == savedSamples);
        mu::SetRendererOverride(nullptr);
    }
}

TEST_CASE("the benchmark preserves requested TAA while it is inactive [benchmark]")
{
    using namespace Core::Benchmark;
    AntialiasingRenderer renderer;
    renderer.temporal = true;
    CHECK_FALSE(renderer.IsTemporalAntialiasingActive());
    mu::SetRendererOverride(&renderer);
    const SceneConfig before = Scene::CaptureCurrentConfig();
    Scene::ApplyConfig(Segments::Find("look.post.off")->config);
    CHECK(renderer.temporal);
    Scene::ApplyConfig(Segments::Find("aa.off")->config);
    CHECK_FALSE(renderer.temporal);
    Scene::ApplyConfig(before);
    CHECK(renderer.temporal);
    const int temporalChanges = renderer.temporalChanges;
    Scene::ApplyConfig(before);
    CHECK(renderer.temporalChanges == temporalChanges);
    mu::SetRendererOverride(nullptr);
}

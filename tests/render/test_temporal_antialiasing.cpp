#include <doctest.h>

#include "Render/PostProcessing/TemporalHistory.h"
#include "Render/PostProcessing/TemporalJitter.h"

#include <glm/gtc/matrix_transform.hpp>

#include <set>
#include <utility>

using namespace Render::PostProcessing;

namespace
{
constexpr std::uint32_t Width = 1280u;
constexpr std::uint32_t Height = 720u;

glm::mat4 Camera(glm::vec3 eye, glm::vec3 target)
{
    const glm::mat4 projection =
        glm::perspective(glm::radians(35.0f), static_cast<float>(Width) / Height, 20.0f, 5000.0f);
    return projection * glm::lookAt(eye, target, glm::vec3(0.0f, 0.0f, 1.0f));
}

glm::mat4 StandingCamera()
{
    return Camera({12000.0f, 11000.0f, 900.0f}, {12400.0f, 11400.0f, 0.0f});
}

// Where `transform` puts `world`, in clip space after the divide by w.
glm::vec3 Project(const glm::mat4& transform, glm::vec3 world)
{
    const glm::vec4 clip = transform * glm::vec4(world, 1.0f);
    return glm::vec3(clip) / clip.w;
}
} // namespace

TEST_CASE("the scene's jitter stays inside its pixel and repeats every 8 frames [render][post_processing][taa]")
{
    std::set<std::pair<float, float>> points;
    for (std::uint32_t frame = 0u; frame < TemporalJitterFrameCount; ++frame)
    {
        const glm::vec2 offset = TemporalJitterPixels(frame);
        CHECK(offset.x >= -0.5f);
        CHECK(offset.x < 0.5f);
        CHECK(offset.y >= -0.5f);
        CHECK(offset.y < 0.5f);
        points.insert({offset.x, offset.y});
    }
    CHECK(points.size() == TemporalJitterFrameCount);
    CHECK(TemporalJitterPixels(TemporalJitterFrameCount) == TemporalJitterPixels(0u));
}

TEST_CASE("the scene's jitter averages near the pixel centre [render][post_processing][taa]")
{
    glm::vec2 sum(0.0f);
    for (std::uint32_t frame = 0u; frame < TemporalJitterFrameCount; ++frame)
    {
        sum += TemporalJitterPixels(frame);
    }
    const glm::vec2 mean = sum / static_cast<float>(TemporalJitterFrameCount);
    CHECK(glm::abs(mean.x) < 0.1f);
    CHECK(glm::abs(mean.y) < 0.1f);
}

TEST_CASE("a jitter in pixels is a clip-space offset with y up [render][post_processing][taa]")
{
    const glm::vec2 clip = TemporalJitterClip({1.0f, 1.0f}, Width, Height);
    CHECK(clip.x == doctest::Approx(2.0f / Width));
    CHECK(clip.y == doctest::Approx(-2.0f / Height));
    CHECK(TemporalJitterClip({0.5f, 0.5f}, 0u, Height) == glm::vec2(0.0f));
}

TEST_CASE("a jittered transform moves what it projects by the jitter [render][post_processing][taa]")
{
    const glm::mat4 camera = StandingCamera();
    const glm::vec2 offset = TemporalJitterClip({0.25f, -0.375f}, Width, Height);
    const glm::vec3 world(12300.0f, 11250.0f, 40.0f);

    const glm::vec3 still = Project(camera, world);
    const glm::vec3 jittered = Project(JitterTransform(camera, offset), world);

    CHECK(jittered.x - still.x == doctest::Approx(offset.x).epsilon(1e-3));
    CHECK(jittered.y - still.y == doctest::Approx(offset.y).epsilon(1e-3));
    CHECK(jittered.z == doctest::Approx(still.z));
}

TEST_CASE("only perspective transforms count as the 3D scene [render][post_processing][taa]")
{
    CHECK(IsPerspective(StandingCamera()));
    CHECK_FALSE(IsPerspective(glm::ortho(0.0f, 640.0f, 480.0f, 0.0f)));
    CHECK_FALSE(IsPerspective(glm::mat4(1.0f)));
}

TEST_CASE("reprojection finds where the previous camera saw a surface [render][post_processing][taa]")
{
    const glm::mat4 previous = StandingCamera();
    const glm::mat4 current = Camera({12030.0f, 11010.0f, 905.0f}, {12430.0f, 11410.0f, 0.0f});
    const glm::vec3 world(12350.0f, 11300.0f, 15.0f);

    const std::optional<glm::mat4> currentToPrevious = CurrentToPreviousClip(previous, current);
    REQUIRE(currentToPrevious.has_value());

    const glm::vec3 now = Project(current, world);
    const glm::vec4 reprojected = *currentToPrevious * glm::vec4(now, 1.0f);
    const glm::vec3 before = Project(previous, world);
    CHECK(reprojected.x / reprojected.w == doctest::Approx(before.x).epsilon(1e-3));
    CHECK(reprojected.y / reprojected.w == doctest::Approx(before.y).epsilon(1e-3));
}

TEST_CASE("a still camera reprojects every surface onto itself [render][post_processing][taa]")
{
    const std::optional<glm::mat4> currentToPrevious = CurrentToPreviousClip(StandingCamera(), StandingCamera());
    REQUIRE(currentToPrevious.has_value());

    const glm::vec4 clip(0.3f, -0.6f, 0.97f, 1.0f);
    const glm::vec4 reprojected = *currentToPrevious * clip;
    CHECK(reprojected.x / reprojected.w == doctest::Approx(clip.x));
    CHECK(reprojected.y / reprojected.w == doctest::Approx(clip.y));
}

TEST_CASE("a camera without an inverse has no reprojection [render][post_processing][taa]")
{
    CHECK_FALSE(CurrentToPreviousClip(StandingCamera(), glm::mat4(0.0f)).has_value());
}

TEST_CASE("the history blends in from the second frame at the same size [render][post_processing][taa]")
{
    TemporalHistory history;
    const TemporalFrame first = history.BeginFrame(StandingCamera(), Width, Height);
    CHECK(first.historyWeight == 0.0f);

    const TemporalFrame second = history.BeginFrame(StandingCamera(), Width, Height);
    CHECK(second.historyWeight == TemporalHistoryWeight);
    CHECK(second.jitterClip != first.jitterClip);

    const TemporalFrame resized = history.BeginFrame(StandingCamera(), Width / 2u, Height / 2u);
    CHECK(resized.historyWeight == 0.0f);
}

TEST_CASE("a frame without temporal anti-aliasing starts the history over [render][post_processing][taa]")
{
    TemporalHistory history;
    (void)history.BeginFrame(StandingCamera(), Width, Height);
    history.Reset();

    CHECK(history.BeginFrame(StandingCamera(), Width, Height).historyWeight == 0.0f);
    CHECK(history.BeginFrame(StandingCamera(), Width, Height).historyWeight == TemporalHistoryWeight);
}

TEST_CASE("a camera without an inverse does not blend the history [render][post_processing][taa]")
{
    TemporalHistory history;
    (void)history.BeginFrame(StandingCamera(), Width, Height);

    CHECK(history.BeginFrame(glm::mat4(0.0f), Width, Height).historyWeight == 0.0f);
}

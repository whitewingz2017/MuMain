#include <doctest.h>

#include "Render/PostProcessing/SkinMotionHistory.h"

#include <array>

using namespace Render::PostProcessing;

namespace
{
// Two bones: identity rotations at `x` and `x + 10` along x.
std::array<float, 24> Bones(float x)
{
    return {1.0f, 0.0f, 0.0f, x,         0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            1.0f, 0.0f, 0.0f, x + 10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
}

SkinPlacement PlacedAt(float x)
{
    SkinPlacement placement;
    placement.bodyOrigin = glm::vec3(x, 0.0f, 0.0f);
    return placement;
}

std::vector<float> AsVector(std::span<const float> bones)
{
    return {bones.begin(), bones.end()};
}

// The addresses of a character's own palette and of one several objects pose into.
int OwnBones = 0;
int SharedBones = 0;
} // namespace

TEST_CASE("a palette's first frame has no motion and its next frame finds it [render][post_processing][taa]")
{
    SkinMotionHistory history;
    const auto before = Bones(100.0f);
    CHECK_FALSE(history.Record(&OwnBones, 1u, before, PlacedAt(5.0f)).has_value());

    history.BeginFrame();
    const std::optional<PreviousSkin> previous = history.Record(&OwnBones, 2u, Bones(103.0f), PlacedAt(6.0f));
    REQUIRE(previous.has_value());
    CHECK(AsVector(previous->bones) == AsVector(before));
    CHECK(previous->placement == PlacedAt(5.0f));
}

TEST_CASE("a palette posed again the same way in a frame keeps its motion [render][post_processing][taa]")
{
    SkinMotionHistory history;
    const auto pose = Bones(100.0f);
    // Each body part of a character transforms the same bones again, under a new version.
    (void)history.Record(&OwnBones, 1u, pose, PlacedAt(5.0f));
    (void)history.Record(&OwnBones, 2u, pose, PlacedAt(5.0f));

    history.BeginFrame();
    CHECK(history.Record(&OwnBones, 3u, Bones(101.0f), PlacedAt(5.0f)).has_value());
}

TEST_CASE("a palette several objects pose into has no motion [render][post_processing][taa]")
{
    SkinMotionHistory history;
    (void)history.Record(&SharedBones, 1u, Bones(100.0f), PlacedAt(5.0f));
    (void)history.Record(&SharedBones, 2u, Bones(900.0f), PlacedAt(5.0f));

    history.BeginFrame();
    CHECK_FALSE(history.Record(&SharedBones, 3u, Bones(100.0f), PlacedAt(5.0f)).has_value());
}

TEST_CASE("one pose drawn at several places finds each place in its turn [render][post_processing][taa]")
{
    SkinMotionHistory history;
    const auto pose = Bones(100.0f);
    (void)history.Record(&OwnBones, 1u, pose, PlacedAt(5.0f));
    (void)history.Record(&OwnBones, 1u, pose, PlacedAt(50.0f));

    history.BeginFrame();
    const std::optional<PreviousSkin> first = history.Record(&OwnBones, 2u, pose, PlacedAt(6.0f));
    const std::optional<PreviousSkin> second = history.Record(&OwnBones, 2u, pose, PlacedAt(51.0f));
    REQUIRE(first.has_value());
    REQUIRE(second.has_value());
    CHECK(first->placement == PlacedAt(5.0f));
    CHECK(second->placement == PlacedAt(50.0f));
    // A place drawn again in the same frame keeps its turn.
    const std::optional<PreviousSkin> again = history.Record(&OwnBones, 2u, pose, PlacedAt(6.0f));
    REQUIRE(again.has_value());
    CHECK(again->placement == PlacedAt(5.0f));
}

TEST_CASE("one pose drawn at many places has no motion [render][post_processing][taa]")
{
    SkinMotionHistory history;
    const auto pose = Bones(100.0f);
    for (int place = 0; place < 20; ++place)
    {
        (void)history.Record(&SharedBones, 1u, pose, PlacedAt(static_cast<float>(place)));
    }

    history.BeginFrame();
    CHECK_FALSE(history.Record(&SharedBones, 2u, pose, PlacedAt(0.0f)).has_value());
}

TEST_CASE("a palette's motion needs every bone the draw uses [render][post_processing][taa]")
{
    SkinMotionHistory history;
    const auto pose = Bones(100.0f);
    const std::span<const float> firstBone(pose.data(), 12u);
    (void)history.Record(&OwnBones, 1u, firstBone, PlacedAt(5.0f));

    history.BeginFrame();
    CHECK_FALSE(history.Record(&OwnBones, 2u, pose, PlacedAt(5.0f)).has_value());
    CHECK(history.Record(&OwnBones, 2u, firstBone, PlacedAt(5.0f)).has_value());
}

TEST_CASE("a mesh using more bones of the same pose extends what the next frame finds [render][post_processing][taa]")
{
    SkinMotionHistory history;
    const auto pose = Bones(100.0f);
    (void)history.Record(&OwnBones, 1u, std::span<const float>(pose.data(), 12u), PlacedAt(5.0f));
    (void)history.Record(&OwnBones, 2u, pose, PlacedAt(5.0f));

    history.BeginFrame();
    const std::optional<PreviousSkin> previous = history.Record(&OwnBones, 3u, pose, PlacedAt(5.0f));
    REQUIRE(previous.has_value());
    CHECK(previous->bones.size() == pose.size());
}

TEST_CASE("a palette missing from a frame, or a reset, starts its motion over [render][post_processing][taa]")
{
    SkinMotionHistory history;
    (void)history.Record(&OwnBones, 1u, Bones(100.0f), PlacedAt(5.0f));
    history.BeginFrame();
    history.BeginFrame();
    CHECK_FALSE(history.Record(&OwnBones, 2u, Bones(100.0f), PlacedAt(5.0f)).has_value());

    history.BeginFrame();
    history.Reset();
    CHECK_FALSE(history.Record(&OwnBones, 3u, Bones(100.0f), PlacedAt(5.0f)).has_value());
}

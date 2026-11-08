#include <doctest.h>

#include "Render/Renderer/SkinnedLightingTransform.h"

#include <glm/gtc/matrix_transform.hpp>

namespace
{
void CheckLighting(const Render::SkinnedLightingTransform::Uniforms& uniforms, const glm::vec3& expectedEye,
                   const glm::mat4& expectedWorld)
{
    for (int axis = 0; axis < 3; ++axis)
        CHECK(uniforms.eyePosition[axis] == doctest::Approx(expectedEye[axis]).epsilon(0.0001));
    for (int row = 0; row < 3; ++row)
    {
        for (int column = 0; column < 4; ++column)
            CHECK(uniforms.vertexToWorldRows[row][column] ==
                  doctest::Approx(expectedWorld[column][row]).epsilon(0.0001));
    }
}
} // namespace

TEST_CASE("lighting initializes even when both views are identity [render][skinned_lighting]")
{
    Render::SkinnedLightingTransform cache;
    CheckLighting(cache.Get(glm::mat4(1.f), glm::mat4(1.f)), glm::vec3(0.f), glm::mat4(1.f));
}

TEST_CASE("lighting follows rotated and scaled model views across repeated meshes [render][skinned_lighting]")
{
    Render::SkinnedLightingTransform cache;
    const glm::vec3 eye(5.f, 7.f, 9.f);
    const glm::mat4 view = glm::lookAt(eye, glm::vec3(0.f), glm::vec3(0.f, 0.f, 1.f));
    glm::mat4 model = glm::translate(glm::mat4(1.f), glm::vec3(3.f, 2.f, -4.f));
    model = glm::rotate(model, glm::radians(30.f), glm::vec3(0.f, 0.f, 1.f));
    model = glm::scale(model, glm::vec3(2.f, 3.f, 4.f));
    const glm::vec3 modelEye = glm::inverse(model) * glm::vec4(eye, 1.f);
    for (int mesh = 0; mesh < 3; ++mesh)
        CheckLighting(cache.Get(view * model, glm::inverse(view)), modelEye, model);
}

TEST_CASE("lighting refreshes when only the scene view changes [render][skinned_lighting]")
{
    Render::SkinnedLightingTransform cache;
    const glm::mat4 modelView = glm::translate(glm::mat4(1.f), glm::vec3(2.f, 3.f, 4.f));
    const glm::vec3 eye(-2.f, -3.f, -4.f);
    CheckLighting(cache.Get(modelView, glm::mat4(1.f)), eye, modelView);
    const glm::mat4 worldFromView = glm::translate(glm::mat4(1.f), glm::vec3(5.f, 6.f, 7.f));
    CheckLighting(cache.Get(modelView, worldFromView), eye, worldFromView * modelView);
}

TEST_CASE("lighting restores the world view after a UI view and follows camera motion [render][skinned_lighting]")
{
    Render::SkinnedLightingTransform cache;
    const glm::vec3 eye(12.f, 8.f, 16.f);
    const glm::mat4 view = glm::lookAt(eye, glm::vec3(0.f), glm::vec3(0.f, 0.f, 1.f));
    const glm::mat4 worldFromView = glm::inverse(view);
    CheckLighting(cache.Get(view, worldFromView), eye, glm::mat4(1.f));
    CheckLighting(cache.Get(glm::mat4(1.f), worldFromView), glm::vec3(0.f), worldFromView);
    CheckLighting(cache.Get(view, worldFromView), eye, glm::mat4(1.f));

    const glm::vec3 movedEye(14.f, 9.f, 16.f);
    const glm::mat4 movedView = glm::lookAt(movedEye, glm::vec3(0.f), glm::vec3(0.f, 0.f, 1.f));
    CheckLighting(cache.Get(movedView, glm::inverse(movedView)), movedEye, glm::mat4(1.f));
}

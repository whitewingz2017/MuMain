#include "stdafx.h"
#include "Render/Effects/GameMasterAura.h"
#include "Render/Effects/ZzzEffect.h"
#include "Render/Renderer/MuRenderer.h"
#include "Render/Renderer/RenderUtils.h"
#include "Render/Terrain/ZzzLodTerrain.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "Engine/Object/ZzzCharacter.h"
#include "UI/Core/WindowSystem.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace Render::Effects::GameMasterAura
{
namespace
{
constexpr float VisibleAlpha = .01f;
constexpr float HaloHeight = 140.f;
constexpr float HaloBehindCharacter = 80.f;
constexpr float HaloDiameter = 155.f;
constexpr float CoreDiameterRatio = .8f;
constexpr float CoreDarkness = .85f;
constexpr float OuterHaloRatio = 1.06f;
constexpr float OuterHaloBrightness = .4f;
constexpr double RotationDegreesPerMillisecond = .012;
constexpr double OrbitRadiansPerMillisecond = -.00032;
constexpr float GroundSealSize = 1.4f;
constexpr float InnerSealRatio = .72f;
constexpr float GroundRingRadius = 88.f;
constexpr float GroundRingWidth = 2.f;
constexpr float GroundRingHeight = 6.f;
constexpr int RingArcCount = 4;
constexpr int SegmentsPerArc = 10;
constexpr int CornersPerQuad = 4;
constexpr float ArcCoverage = .6f;
constexpr int ShardCount = 8;
constexpr float ShardOrbitRadius = 70.f;
constexpr float ShardHeight = 45.f;
constexpr float ShardBob = 10.f;
constexpr float ShardHalfWidth = 5.f;
constexpr float ShardHalfHeight = 13.f;
constexpr float ShardGlowDiameter = 23.f;
constexpr float ShardFacetShade = .12f;
constexpr double SparkleRadiansPerMillisecond = .006;
constexpr float SparkleDiameter = 18.f;
constexpr float SparkleRotationStep = 45.f;
constexpr int RisingRingCount = 3;
constexpr int RisingRingSegments = 32;
constexpr double RisingRingPeriod = 2600.;
constexpr float RisingRingHeight = 200.f;
constexpr float RisingRingRadius = 66.f;
constexpr float RisingRingExpansion = 20.f;
constexpr float RisingRingWidth = 1.8f;
constexpr int BeamCount = 8;
constexpr int BeamLayers = 2;
constexpr double BeamPeriod = 1700.;
constexpr float BeamRadius = 80.f;
constexpr float BeamRise = 220.f;
constexpr float BeamLength = 65.f;
constexpr float BeamGlowWidth = 5.f;
constexpr float BeamCoreWidth = 1.f;
constexpr float BeamGlowBrightness = .25f;
constexpr float TrailBehind = 22.f;
constexpr float TrailSide = 15.f;
constexpr float TrailHeight = 12.f;
constexpr float TrailGlowScale = .7f;
constexpr float TrailSparkleScale = .28f;
constexpr int FadingLightSubtype = 1;
constexpr int TrailSparkleSubtype = 0;
constexpr double FullTurn = 2. * std::numbers::pi;
constexpr std::array<float, 3> Violet = {.65f, .3f, 1.f};
constexpr std::array<float, 3> IceViolet = {.8f, .65f, 1.f};

bool ShouldRender(const CHARACTER* character, const OBJECT* object)
{
    if (!character || !object || object->Kind != KIND_PLAYER || object->Type != MODEL_PLAYER
        || !object->Live || !object->Visible || object->Alpha <= VisibleAlpha || object->Scale <= 0.f
        || !g_pOption->GetRenderAllEffects())
        return false;
    const bool isOperator = (character->CtlCode & (CTLCODE_08OPERATOR | CTLCODE_20OPERATOR)) != 0;
    return (isOperator || object->SubType == MODEL_GM_CHARACTER || g_isCharacterBuff(object, eBuff_GMEffect))
        && !g_isCharacterBuff(object, eBuff_Cloaking) && !g_isCharacterBuff(object, eBuff_CrywolfNPCHide);
}

float Rotation()
{
    return static_cast<float>(std::fmod(WorldTime * RotationDegreesPerMillisecond, 360.));
}

std::array<float, 3> ShardPosition(const OBJECT* object, int index)
{
    const double angle = std::fmod(WorldTime * OrbitRadiansPerMillisecond, FullTurn) + index * FullTurn / ShardCount;
    return {object->Position[0] + static_cast<float>(std::cos(angle)) * ShardOrbitRadius * object->Scale,
            object->Position[1] + static_cast<float>(std::sin(angle)) * ShardOrbitRadius * object->Scale,
            object->Position[2] + (ShardHeight + static_cast<float>(std::sin(angle * 2.)) * ShardBob) * object->Scale};
}

void RenderHalo(const OBJECT* object)
{
    RenderWithSpriteCamera([object]
    {
        vec3_t position, light;
        VectorCopy(object->Position, position);
        position[2] += HaloHeight * object->Scale;
        const float dx = object->Position[0] - g_Camera.Position[0];
        const float dy = object->Position[1] - g_Camera.Position[1];
        const float dz = position[2] - g_Camera.Position[2];
        const float distance = std::hypot(dx, dy, dz);
        if (distance > 0.f)
        {
            position[0] += dx / distance * HaloBehindCharacter * object->Scale;
            position[1] += dy / distance * HaloBehindCharacter * object->Scale;
            position[2] += dz / distance * HaloBehindCharacter * object->Scale;
        }
        const float diameter = HaloDiameter * object->Scale;
        const float darkness = CoreDarkness * object->Alpha;
        Vector(darkness, darkness, darkness, light);
        EnableAlphaBlendMinus();
        RenderSprite(BITMAP_LIGHT, position, diameter * CoreDiameterRatio, diameter * CoreDiameterRatio, light);
        Vector(Violet[0] * object->Alpha, Violet[1] * object->Alpha, Violet[2] * object->Alpha, light);
        EnableAlphaBlend();
        RenderSprite(BITMAP_MAGIC, position, diameter, diameter, light, Rotation());
        VectorScale(light, OuterHaloBrightness, light);
        RenderSprite(BITMAP_MAGIC, position, diameter * OuterHaloRatio, diameter * OuterHaloRatio, light, -Rotation());
    });
}

void RenderShardSparkles(const OBJECT* object)
{
    RenderWithSpriteCamera([object]
    {
        EnableAlphaBlend();
        for (int index = 0; index < ShardCount; ++index)
        {
            const auto shard = ShardPosition(object, index);
            vec3_t position = {shard[0], shard[1], shard[2]};
            const float pulse = static_cast<float>(.5 + .5 * std::sin(WorldTime * SparkleRadiansPerMillisecond + index));
            const float brightness = (.3f + .7f * pulse * pulse) * object->Alpha;
            vec3_t light = {IceViolet[0] * brightness, IceViolet[1] * brightness, brightness};
            const float glowSize = ShardGlowDiameter * object->Scale;
            RenderSprite(BITMAP_LIGHT, position, glowSize, glowSize, light);
            const float sparkleSize = SparkleDiameter * (.5f + pulse) * object->Scale;
            RenderSprite(BITMAP_SHINY + 1, position, sparkleSize, sparkleSize, light, Rotation() + index * SparkleRotationStep);
        }
    });
}

void RenderRisingRings(const OBJECT* object)
{
    std::array<mu::Vertex3D, RisingRingCount * RisingRingSegments * CornersPerQuad> vertices{};
    std::size_t vertex = 0;
    for (int ring = 0; ring < RisingRingCount; ++ring)
    {
        const float phase = static_cast<float>(std::fmod(WorldTime / RisingRingPeriod + double(ring) / RisingRingCount, 1.));
        const float brightness = static_cast<float>(std::sin(phase * std::numbers::pi)) * object->Alpha;
        const auto color = mu::PackABGR(Violet[0] * brightness, Violet[1] * brightness, brightness, brightness);
        const float height = object->Position[2] + (GroundRingHeight + phase * RisingRingHeight) * object->Scale;
        for (int segment = 0; segment < RisingRingSegments; ++segment)
        {
            for (int corner = 0; corner < CornersPerQuad; ++corner)
            {
                const double angle = (segment + (corner == 1 || corner == 2 ? 1 : 0)) * FullTurn / RisingRingSegments;
                const float radius = (RisingRingRadius + phase * RisingRingExpansion
                    + (corner >= 2 ? RisingRingWidth : 0.f)) * object->Scale;
                vertices[vertex++] = {object->Position[0] + static_cast<float>(std::cos(angle)) * radius,
                    object->Position[1] + static_cast<float>(std::sin(angle)) * radius, height,
                    0.f, 0.f, 1.f, 0.f, 0.f, color};
            }
        }
    }
    EnableAlphaBlend();
    DisableTexture();
    DisableDepthMask();
    mu::GetRenderer().RenderQuad3D(vertices, 0u);
}

void RenderRisingBeams(const OBJECT* object)
{
    std::array<mu::Vertex3D, BeamCount * BeamLayers * CornersPerQuad> vertices{};
    const float dx = object->Position[0] - g_Camera.Position[0];
    const float dy = object->Position[1] - g_Camera.Position[1];
    const float distance = std::hypot(dx, dy);
    const float rightX = distance > 0.f ? -dy / distance : 1.f;
    const float rightY = distance > 0.f ? dx / distance : 0.f;
    std::size_t vertex = 0;
    for (int beam = 0; beam < BeamCount; ++beam)
    {
        // Neighbouring beams rise on opposite beats.
        const float phase = static_cast<float>(std::fmod(WorldTime / BeamPeriod + (beam % 2) * .5, 1.));
        const double angle = beam * FullTurn / BeamCount;
        const float x = object->Position[0] + static_cast<float>(std::cos(angle)) * BeamRadius * object->Scale;
        const float y = object->Position[1] + static_cast<float>(std::sin(angle)) * BeamRadius * object->Scale;
        const float head = object->Position[2] + (GroundRingHeight + phase * BeamRise) * object->Scale;
        const float tail = std::max(object->Position[2] + GroundRingHeight * object->Scale, head - BeamLength * object->Scale);
        const float pulse = static_cast<float>(std::sin(phase * std::numbers::pi)) * object->Alpha;
        const auto& tint = beam % 2 == 0 ? Violet : IceViolet;
        for (int layer = 0; layer < BeamLayers; ++layer)
        {
            const float width = (layer == 0 ? BeamGlowWidth : BeamCoreWidth) * object->Scale;
            const float brightness = pulse * (layer == 0 ? BeamGlowBrightness : 1.f);
            const auto color = mu::PackABGR(tint[0] * brightness, tint[1] * brightness, brightness, brightness);
            vertices[vertex++] = {x - rightX * width, y - rightY * width, tail, 0.f, 0.f, 1.f, 0.f, 0.f, 0u};
            vertices[vertex++] = {x + rightX * width, y + rightY * width, tail, 0.f, 0.f, 1.f, 0.f, 0.f, 0u};
            vertices[vertex++] = {x + rightX * width, y + rightY * width, head, 0.f, 0.f, 1.f, 0.f, 0.f, color};
            vertices[vertex++] = {x - rightX * width, y - rightY * width, head, 0.f, 0.f, 1.f, 0.f, 0.f, color};
        }
    }
    mu::GetRenderer().RenderQuad3D(vertices, 0u);
}

void EmitWalkingTrail(const CHARACTER* character, const OBJECT* object)
{
    if (!character->Movement)
        return;
    // The walking direction is the facing the movement update keeps.
    const double heading = object->Angle[2] * std::numbers::pi / 180.;
    const float sine = static_cast<float>(std::sin(heading));
    const float cosine = static_cast<float>(std::cos(heading));
    vec3_t light = {Violet[0] * object->Alpha, Violet[1] * object->Alpha, object->Alpha};
    vec3_t angle;
    VectorCopy(object->Angle, angle);
    for (const float side : {-TrailSide, TrailSide})
    {
        vec3_t position = {object->Position[0] + (-sine * TrailBehind + cosine * side) * object->Scale,
            object->Position[1] + (cosine * TrailBehind + sine * side) * object->Scale,
            object->Position[2] + TrailHeight * object->Scale};
        CreateParticleFpsChecked(BITMAP_LIGHT, position, angle, light, FadingLightSubtype, TrailGlowScale * object->Scale);
        CreateParticleFpsChecked(BITMAP_SHINY, position, angle, light, TrailSparkleSubtype, TrailSparkleScale * object->Scale);
    }
}

void RenderGroundRings(const OBJECT* object)
{
    vec3_t light = {Violet[0] * object->Alpha, Violet[1] * object->Alpha, Violet[2] * object->Alpha};
    const float size = GroundSealSize * object->Scale;
    EnableAlphaBlend();
    RenderTerrainAlphaBitmap(BITMAP_MAGIC, object->Position[0], object->Position[1], size, size, light, Rotation());
    RenderTerrainAlphaBitmap(BITMAP_MAGIC, object->Position[0], object->Position[1], size * InnerSealRatio,
        size * InnerSealRatio, light, -Rotation());
    DisableTexture();
    DisableDepthMask();
    const auto color = mu::PackABGR(light[0], light[1], light[2], object->Alpha);
    std::array<mu::Vertex3D, RingArcCount * SegmentsPerArc * CornersPerQuad> vertices{};
    std::size_t vertex = 0;
    for (int arc = 0; arc < RingArcCount; ++arc)
    {
        for (int segment = 0; segment < SegmentsPerArc; ++segment)
        {
            for (int corner = 0; corner < CornersPerQuad; ++corner)
            {
                const bool next = corner == 1 || corner == 2;
                const bool outer = corner >= 2;
                const double angle = WorldTime * -OrbitRadiansPerMillisecond + arc * FullTurn / RingArcCount
                    + (segment + (next ? 1 : 0)) * FullTurn / RingArcCount * ArcCoverage / SegmentsPerArc;
                const float radius = (GroundRingRadius + (outer ? GroundRingWidth : 0.f)) * object->Scale;
                const float x = object->Position[0] + static_cast<float>(std::cos(angle)) * radius;
                const float y = object->Position[1] + static_cast<float>(std::sin(angle)) * radius;
                vertices[vertex++] = {x, y, RequestTerrainHeight(x, y) + GroundRingHeight, 0.f, 0.f, 1.f, 0.f, 0.f, color};
            }
        }
    }
    mu::GetRenderer().RenderQuad3D(vertices, 0u);
}

void RenderShards(const OBJECT* object)
{
    constexpr int FacesPerShard = 8;
    constexpr int EquatorCorners = 4;
    constexpr int VerticesPerFace = 3;
    std::array<mu::Vertex3D, ShardCount * FacesPerShard * VerticesPerFace> vertices{};
    const float width = ShardHalfWidth * object->Scale;
    const float height = ShardHalfHeight * object->Scale;
    const std::array<std::array<float, 3>, 6> corners = {{{0.f, 0.f, height}, {0.f, 0.f, -height},
        {width, 0.f, 0.f}, {0.f, width, 0.f}, {-width, 0.f, 0.f}, {0.f, -width, 0.f}}};
    std::size_t vertex = 0;
    for (int index = 0; index < ShardCount; ++index)
    {
        const auto position = ShardPosition(object, index);
        for (int face = 0; face < FacesPerShard; ++face)
        {
            const float shade = 1.f - (face % EquatorCorners) * ShardFacetShade;
            const auto color = mu::PackABGR(Violet[0] * shade, Violet[1] * shade, Violet[2] * shade, object->Alpha);
            const std::array<int, VerticesPerFace> triangle = {face / EquatorCorners, 2 + face % EquatorCorners,
                2 + (face + 1) % EquatorCorners};
            for (const int corner : triangle)
                vertices[vertex++] = {position[0] + corners[corner][0], position[1] + corners[corner][1],
                    position[2] + corners[corner][2], 0.f, 0.f, 1.f, 0.f, 0.f, color};
        }
    }
    EnableAlphaBlend3();
    DisableTexture();
    DisableDepthMask();
    mu::GetRenderer().RenderTriangles(vertices, 0u);
}
}

void RenderBehind(const CHARACTER* character, const OBJECT* object)
{
    if (!ShouldRender(character, object))
        return;
    // Draw the seal first so opaque armor and wings cover its centre.
    RenderHalo(object);
    DisableAlphaBlend();
}

void Render(const CHARACTER* character, const OBJECT* object)
{
    if (!ShouldRender(character, object))
        return;

    RenderGroundRings(object);
    RenderRisingRings(object);
    RenderRisingBeams(object);
    RenderShards(object);
    RenderShardSparkles(object);
    EmitWalkingTrail(character, object);
    DisableAlphaBlend();
}
}

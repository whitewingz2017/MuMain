#include "stdafx.h"
#include "Render/Effects/RankAura.h"

#include "Core/Utilities/Random.h"
#include "Data/GameData/RankTitles/RankTitles.h"
#include "Engine/Object/ZzzCharacter.h"
#include "GameLogic/RankTitles/RankTitleStore.h"
#include "Render/Effects/ZzzEffect.h"
#include "Render/Renderer/MuRenderer.h"
#include "Render/Renderer/RenderUtils.h"
#include "Render/Terrain/ZzzLodTerrain.h"
#include "Render/Textures/ImageByPath.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "UI/Core/WindowSystem.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

namespace Render::Effects::RankAura
{
namespace
{
using ::Data::RankTitles::AuraBlend;
using ::Data::RankTitles::AuraLayer;
using ::Data::RankTitles::AuraLayerType;
using ::Data::RankTitles::EmitWhen;
using ::Data::RankTitles::RankTitle;

constexpr float VisibleAlpha = .01f;
// The dimmest a pulsing layer gets, of its full brightness.
constexpr float PulseFloor = .35f;
constexpr double MillisecondsPerSecond = 1000.;
constexpr double FullTurnDegrees = 360.;
constexpr double FullTurn = 2. * std::numbers::pi;
constexpr double RadiansPerDegree = std::numbers::pi / 180.;
constexpr int RingSegments = 32;
constexpr float HalfWidth = .5f;
// Neighbouring beams rise on opposite beats.
constexpr double NeighbourBeamOffset = .5;

// The vertices of the untextured layers, kept between frames: only the main render thread draws.
std::vector<mu::Vertex3D> vertices;

const RankTitle* AuraTitle(const CHARACTER* character, const OBJECT* object)
{
    if (!character || !object || object->Kind != KIND_PLAYER || object->Type != MODEL_PLAYER || !object->Live
        || !object->Visible || object->Alpha <= VisibleAlpha || object->Scale <= 0.f
        || !g_pOption->GetRenderAllEffects())
        return nullptr;
    if (g_isCharacterBuff(object, eBuff_Cloaking) || g_isCharacterBuff(object, eBuff_CrywolfNPCHide))
        return nullptr;
    const RankTitle* title = ::Data::RankTitles::Find(::GameLogic::RankTitles::RankOf(character));
    return title && !title->aura.empty() ? title : nullptr;
}

// Degrees turned at `degreesPerSecond`.
float Turned(float degreesPerSecond)
{
    return static_cast<float>(std::fmod(WorldTime * degreesPerSecond / MillisecondsPerSecond, FullTurnDegrees));
}

// Where in its cycle of `period` milliseconds something is, from 0 to 1; `offset` shifts it.
float Phase(float period, double offset = 0.)
{
    return static_cast<float>(std::fmod(WorldTime / period + offset, 1.));
}

float Pulse(const AuraLayer& layer, double offset = 0.)
{
    if (layer.pulse <= 0.f)
        return 1.f;
    const float wave = .5f + .5f * static_cast<float>(std::sin(Phase(layer.pulse, offset) * FullTurn));
    return PulseFloor + (1.f - PulseFloor) * wave;
}

// A cycle's brightness: dark as it starts and ends, full in the middle.
float Swell(float phase)
{
    return static_cast<float>(std::sin(phase * std::numbers::pi));
}

void Light(const AuraLayer& layer, float brightness, vec3_t light)
{
    Vector(layer.color[0] * brightness, layer.color[1] * brightness, layer.color[2] * brightness, light);
}

std::uint32_t PackedColor(const AuraLayer& layer, float brightness)
{
    return mu::PackABGR(layer.color[0] * brightness, layer.color[1] * brightness, layer.color[2] * brightness,
                        brightness);
}

int TextureOf(const AuraLayer& layer)
{
    if (layer.texture.sprite >= 0)
        return layer.texture.sprite;
    return static_cast<int>(::Render::Textures::ImageByPath::Get(layer.texture.image));
}

void ApplyBlend(const AuraLayer& layer)
{
    if (layer.blend == AuraBlend::Subtract)
        EnableAlphaBlendMinus();
    else
        EnableAlphaBlend();
}

void PushAwayFromCamera(vec3_t position, float distance)
{
    const float dx = position[0] - g_Camera.Position[0];
    const float dy = position[1] - g_Camera.Position[1];
    const float dz = position[2] - g_Camera.Position[2];
    const float length = std::hypot(dx, dy, dz);
    if (length <= 0.f)
        return;
    position[0] += dx / length * distance;
    position[1] += dy / length * distance;
    position[2] += dz / length * distance;
}

void AddVertex(const OBJECT* object, double angle, float radius, float z, std::uint32_t color)
{
    vertices.push_back({object->Position[0] + static_cast<float>(std::cos(angle)) * radius,
                        object->Position[1] + static_cast<float>(std::sin(angle)) * radius, z, 0.f, 0.f, 1.f, 0.f,
                        0.f, color});
}

void DrawUntextured(const AuraLayer& layer)
{
    if (vertices.empty())
        return;
    ApplyBlend(layer);
    DisableTexture();
    DisableDepthMask();
    mu::GetRenderer().RenderQuad3D(vertices, 0u);
}

void RenderHalo(const AuraLayer& layer, const OBJECT* object)
{
    const int texture = TextureOf(layer);
    if (texture == BITMAP_UNKNOWN)
        return;
    RenderWithSpriteCamera([&layer, object, texture]
    {
        vec3_t position, light;
        VectorCopy(object->Position, position);
        position[2] += layer.height * object->Scale;
        PushAwayFromCamera(position, layer.depth * object->Scale);
        Light(layer, Pulse(layer) * object->Alpha, light);
        const float diameter = layer.size * object->Scale;
        ApplyBlend(layer);
        RenderSprite(texture, position, diameter, diameter, light, Turned(layer.spin));
    });
}

void RenderGroundSeal(const AuraLayer& layer, const OBJECT* object)
{
    const int texture = TextureOf(layer);
    if (texture == BITMAP_UNKNOWN)
        return;
    vec3_t light;
    Light(layer, Pulse(layer) * object->Alpha, light);
    const float size = layer.scale * object->Scale;
    ApplyBlend(layer);
    RenderTerrainAlphaBitmap(texture, object->Position[0], object->Position[1], size, size, light, Turned(layer.spin));
}

void AddRing(const AuraLayer& layer, const OBJECT* object, float phase)
{
    const auto color = PackedColor(layer, Swell(phase) * Pulse(layer) * object->Alpha);
    const float z = object->Position[2] + (layer.height + phase * layer.rise) * object->Scale;
    const float inner = (layer.radius + phase * layer.spread) * object->Scale;
    const float outer = inner + layer.width * object->Scale;
    for (int segment = 0; segment < RingSegments; ++segment)
    {
        const double from = segment * FullTurn / RingSegments;
        const double to = (segment + 1) * FullTurn / RingSegments;
        AddVertex(object, from, inner, z, color);
        AddVertex(object, to, inner, z, color);
        AddVertex(object, to, outer, z, color);
        AddVertex(object, from, outer, z, color);
    }
}

void RenderRisingRings(const AuraLayer& layer, const OBJECT* object)
{
    vertices.clear();
    for (int ring = 0; ring < layer.count; ++ring)
        AddRing(layer, object, Phase(layer.period, static_cast<double>(ring) / layer.count));
    DrawUntextured(layer);
}

// A beam fades from nothing at its tail to the layer's color at its head, facing the camera.
void AddBeam(const AuraLayer& layer, const OBJECT* object, int beam, float rightX, float rightY)
{
    const float phase = Phase(layer.period, (beam % 2) * NeighbourBeamOffset);
    const double angle = Turned(layer.spin) * RadiansPerDegree + beam * FullTurn / layer.count;
    const float x = object->Position[0] + static_cast<float>(std::cos(angle)) * layer.radius * object->Scale;
    const float y = object->Position[1] + static_cast<float>(std::sin(angle)) * layer.radius * object->Scale;
    const float base = object->Position[2] + layer.height * object->Scale;
    const float head = base + phase * layer.rise * object->Scale;
    const float tail = std::max(base, head - layer.length * object->Scale);
    const float half = layer.width * HalfWidth * object->Scale;
    const auto color = PackedColor(layer, Swell(phase) * Pulse(layer) * object->Alpha);
    vertices.push_back({x - rightX * half, y - rightY * half, tail, 0.f, 0.f, 1.f, 0.f, 0.f, 0u});
    vertices.push_back({x + rightX * half, y + rightY * half, tail, 0.f, 0.f, 1.f, 0.f, 0.f, 0u});
    vertices.push_back({x + rightX * half, y + rightY * half, head, 0.f, 0.f, 1.f, 0.f, 0.f, color});
    vertices.push_back({x - rightX * half, y - rightY * half, head, 0.f, 0.f, 1.f, 0.f, 0.f, color});
}

void RenderBeams(const AuraLayer& layer, const OBJECT* object)
{
    const float dx = object->Position[0] - g_Camera.Position[0];
    const float dy = object->Position[1] - g_Camera.Position[1];
    const float distance = std::hypot(dx, dy);
    const float rightX = distance > 0.f ? -dy / distance : 1.f;
    const float rightY = distance > 0.f ? dx / distance : 0.f;
    vertices.clear();
    for (int beam = 0; beam < layer.count; ++beam)
        AddBeam(layer, object, beam, rightX, rightY);
    DrawUntextured(layer);
}

void RenderOrbit(const AuraLayer& layer, const OBJECT* object)
{
    const int texture = TextureOf(layer);
    if (texture == BITMAP_UNKNOWN)
        return;
    RenderWithSpriteCamera([&layer, object, texture]
    {
        ApplyBlend(layer);
        const double turn = Turned(layer.spin) * RadiansPerDegree;
        const float size = layer.size * object->Scale;
        for (int index = 0; index < layer.count; ++index)
        {
            const double angle = turn + index * FullTurn / layer.count;
            const float bob = static_cast<float>(std::sin(angle * 2.)) * layer.bob;
            vec3_t position = {
                object->Position[0] + static_cast<float>(std::cos(angle)) * layer.radius * object->Scale,
                object->Position[1] + static_cast<float>(std::sin(angle)) * layer.radius * object->Scale,
                object->Position[2] + (layer.height + bob) * object->Scale};
            vec3_t light;
            Light(layer, Pulse(layer, static_cast<double>(index) / layer.count) * object->Alpha, light);
            RenderSprite(texture, position, size, size, light);
        }
    });
}

void EmitParticles(const AuraLayer& layer, const CHARACTER* character, const OBJECT* object)
{
    if (layer.when == EmitWhen::Moving && !character->Movement)
        return;
    vec3_t light, angle;
    Light(layer, Pulse(layer) * object->Alpha, light);
    VectorCopy(object->Angle, angle);
    for (int index = 0; index < layer.count; ++index)
    {
        if (Random::Unit() > layer.chance)
            continue;
        const double around = Random::UnitDouble() * FullTurn;
        const float distance = Random::RangeFloat(0.f, layer.radius) * object->Scale;
        const float lift = layer.height + Random::RangeFloat(0.f, layer.rise);
        vec3_t position = {object->Position[0] + static_cast<float>(std::cos(around)) * distance,
                           object->Position[1] + static_cast<float>(std::sin(around)) * distance,
                           object->Position[2] + lift * object->Scale};
        CreateParticleFpsChecked(layer.particle, position, angle, light, layer.subType, layer.scale * object->Scale);
    }
}

void RenderLayer(const AuraLayer& layer, const OBJECT* object)
{
    switch (layer.type)
    {
    case AuraLayerType::GroundSeal:
        RenderGroundSeal(layer, object);
        break;
    case AuraLayerType::RisingRings:
        RenderRisingRings(layer, object);
        break;
    case AuraLayerType::Beams:
        RenderBeams(layer, object);
        break;
    case AuraLayerType::Orbit:
        RenderOrbit(layer, object);
        break;
    case AuraLayerType::Halo:
    case AuraLayerType::Particles:
        break;
    }
}
} // namespace

void Update(const CHARACTER* character, const OBJECT* object)
{
    const RankTitle* title = AuraTitle(character, object);
    if (!title)
        return;
    for (const AuraLayer& layer : title->aura)
    {
        if (layer.type == AuraLayerType::Particles)
            EmitParticles(layer, character, object);
    }
}

void RenderBehind(const CHARACTER* character, const OBJECT* object)
{
    const RankTitle* title = AuraTitle(character, object);
    if (!title)
        return;
    for (const AuraLayer& layer : title->aura)
    {
        if (layer.type == AuraLayerType::Halo)
            RenderHalo(layer, object);
    }
    DisableAlphaBlend();
}

void Render(const CHARACTER* character, const OBJECT* object)
{
    const RankTitle* title = AuraTitle(character, object);
    if (!title)
        return;
    for (const AuraLayer& layer : title->aura)
        RenderLayer(layer, object);
    DisableAlphaBlend();
}
} // namespace Render::Effects::RankAura

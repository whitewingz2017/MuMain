#include "stdafx.h"
#include "ItemMeshLayers.h"
#include "ItemModelLookup.h"
#include "ScopedItemEffects.h"
#include "Render/Models/ZzzBMD.h"
#include "Engine/Object/w_ObjectInfo.h"
#include <cstdlib>

namespace Render::Items
{
namespace
{
void DrawLayer(BMD* model, const Data::Items::ItemMeshLayer& layer, float alpha)
{
    const bool flickers = layer.light[0] == Data::Items::RandomEffectCurve
        || std::any_of(layer.color.begin(), layer.color.end(), [](const auto& curve)
            { return curve[0] == Data::Items::RandomEffectCurve; });
    const int random = flickers ? std::rand() : 0;
    const float light = static_cast<float>(Data::Items::EvaluateItemEffectCurve(layer.light, WorldTime, random));
    if (layer.light[0] == Data::Items::FrameGateEffectCurve && light == 0.0f) return;
    for (int channel = 0; channel < 3; ++channel)
        model->BodyLight[channel] = static_cast<float>(Data::Items::EvaluateItemEffectCurve(layer.color[channel], WorldTime, random));
    model->RenderMesh(layer.mesh, layer.flags, alpha, layer.blend,
        light,
        static_cast<float>(Data::Items::EvaluateItemEffectCurve(layer.u, WorldTime)),
        static_cast<float>(Data::Items::EvaluateItemEffectCurve(layer.v, WorldTime)));
}
}

bool RenderMeshLayers(BMD* model, const OBJECT* object, int type, float alpha, int flags)
{
    const auto* definition = FindItemModel(type);
    if (!definition || definition->material.layers.empty() || !(flags & RENDER_TEXTURE)
        || object->EnableShadow || (flags & RENDER_SHADOWMAP)) return false;
    vec3_t originalLight;
    VectorCopy(model->BodyLight, originalLight);
    model->BeginRender(alpha);
    for (const auto& layer : definition->material.layers)
    {
        if (layer.mesh < 0 || layer.mesh >= model->NumMeshs || layer.mesh == object->HiddenMesh) continue;
        if (!ScopedItemEffects::Enabled() && (layer.flags & RENDER_BRIGHT)) continue;
        DrawLayer(model, layer, alpha);
    }
    model->EndRender();
    VectorCopy(originalLight, model->BodyLight);
    return true;
}
}

#include "stdafx.h"

#include "ItemModelMaterialJson.h"
#include "ItemModelValueReader.h"

#include <string>

namespace Data::Items::MaterialJson
{
namespace
{
using Json::OrderedJson;

constexpr const char* SpecularKey = "specular";
constexpr const char* AdditiveMeshesKey = "additiveMeshes";
constexpr const char* AnimationsKey = "animations";
constexpr const char* LayersKey = "layers";

bool ReadCurve(ModelJson::ItemModelValueReader& reader, const char* key, ItemEffectCurve& curve)
{
    constexpr double MaxParameter = 1000000.0;
    constexpr double MaxAtlasPeriod = 4096.0;
    const auto* value = reader.ReadJson(key);
    if (!value) return true;
    if (value->is_number())
    {
        curve = {0, value->get<double>()};
        return std::isfinite(curve[1]) && std::abs(curve[1]) <= MaxParameter;
    }
    if (!reader.ReadNumbers(key, curve, curve.size())) return false;
    if (curve[0] != std::floor(curve[0]) || curve[0] < 0 || curve[0] > FrameGateEffectCurve || curve[4] < 0) return false;
    if ((curve[0] == 3 || curve[0] == 4) && (curve[4] <= 0 || curve[4] > MaxAtlasPeriod)) return false;
    if ((curve[0] == RandomEffectCurve || curve[0] == FrameGateEffectCurve)
        && (curve[4] < 1 || curve[4] > MaxAtlasPeriod || curve[4] != std::floor(curve[4]))) return false;
    return std::all_of(curve.begin(), curve.end(), [](double number)
        { return std::isfinite(number) && std::abs(number) <= MaxParameter; });
}

void ReadLayers(ModelJson::ItemModelValueReader& reader, ItemMaterial& material,
                const ModelJson::ReportIssue& report)
{
    const auto* rows = reader.ReadJson(LayersKey);
    if (!rows) return;
    constexpr int MaxLayers = 64;
    constexpr int MaxMesh = 31;
    constexpr int MaxFlags = 131071;
    if (!rows->is_array() || rows->size() > MaxLayers)
    {
        reader.Error(LayersKey, "must contain at most 64 layers");
        return;
    }
    for (const auto& row : *rows)
    {
        ModelJson::ItemModelValueReader value(row, LayersKey, report);
        ItemMeshLayer layer;
        if (!row.is_object() || !value.ReadIndex("mesh", layer.mesh, MaxMesh)
            || !value.ReadIndex("flags", layer.flags, MaxFlags)
            || !value.ReadInteger("blend", layer.blend, -2, MaxMesh)
            || !ReadCurve(value, "light", layer.light) || !ReadCurve(value, "u", layer.u)
            || !ReadCurve(value, "v", layer.v) || !ReadCurve(value, "red", layer.color[0])
            || !ReadCurve(value, "green", layer.color[1]) || !ReadCurve(value, "blue", layer.color[2]))
        {
            reader.Error(LayersKey, "invalid mesh, flags, blend or animation curve");
            continue;
        }
        value.WarnAboutUnknownKeys();
        material.layers.push_back(layer);
    }
}

OrderedJson WriteCurve(const ItemEffectCurve& curve)
{
    return curve[0] == 0 ? OrderedJson(curve[1]) : OrderedJson(curve);
}

void WriteLayers(const ItemMaterial& material, OrderedJson& json)
{
    if (material.layers.empty()) return;
    json[LayersKey] = OrderedJson::array();
    for (const auto& layer : material.layers)
        json[LayersKey].push_back({{"mesh", layer.mesh}, {"flags", layer.flags}, {"blend", layer.blend},
            {"light", WriteCurve(layer.light)}, {"u", WriteCurve(layer.u)}, {"v", WriteCurve(layer.v)},
            {"red", WriteCurve(layer.color[0])}, {"green", WriteCurve(layer.color[1])}, {"blue", WriteCurve(layer.color[2])}});
}

void ReadAnimations(ModelJson::ItemModelValueReader& reader, ItemMaterial& material,
                    const ModelJson::ReportIssue& report)
{
    const auto* rows = reader.ReadJson(AnimationsKey);
    if (!rows) return;
    constexpr int MaxMeshes = 32;
    constexpr int MaxTiles = 16;
    if (!rows->is_array() || rows->size() > MaxMeshes)
    {
        reader.Error(AnimationsKey, "must contain at most 32 animations");
        return;
    }
    for (const auto& row : *rows)
    {
        ModelJson::ItemModelValueReader value(row, AnimationsKey, report);
        ItemMeshAnimation animation;
        if (!row.is_object() || !value.ReadIndex("mesh", animation.mesh, MaxMeshes - 1)
            || !value.ReadInteger("columns", animation.columns, 1, MaxTiles)
            || !value.ReadInteger("rows", animation.rows, 1, MaxTiles)
            || !value.ReadInteger("frames", animation.frames, 1, animation.columns * animation.rows))
        {
            reader.Error(AnimationsKey, "invalid mesh, columns, rows or frame count");
            continue;
        }
        material.animations.push_back(animation);
    }
}

void ReadAdditiveMeshes(ModelJson::ItemModelValueReader& reader, ItemMaterial& material)
{
    constexpr int MaxMeshes = 32;
    reader.ReadIndexes(AdditiveMeshesKey, material.additiveMeshes, MaxMeshes - 1);
    if (material.additiveMeshes.size() > MaxMeshes)
    {
        reader.Error(AdditiveMeshesKey, "must contain at most 32 mesh indexes");
        material.additiveMeshes.clear();
    }
}
} // namespace

void Write(const ItemModelDefinition& model, OrderedJson& json)
{
    if (model.material == ItemMaterial{})
    {
        return;
    }

    OrderedJson material = OrderedJson::object();
    material[SpecularKey] = ModelJson::WriteNumber(model.material.specular);
    WriteLayers(model.material, material);
    if (!model.material.additiveMeshes.empty())
        material[AdditiveMeshesKey] = model.material.additiveMeshes;
    if (!model.material.animations.empty())
    {
        material[AnimationsKey] = OrderedJson::array();
        for (const auto& animation : model.material.animations)
            material[AnimationsKey].push_back({{"mesh", animation.mesh}, {"columns", animation.columns},
                {"rows", animation.rows}, {"frames", animation.frames}});
    }
    json[MaterialKey] = std::move(material);
}

void Read(const OrderedJson& json, ItemModelDefinition& model, const ModelJson::ReportIssue& report)
{
    const OrderedJson* object = ModelJson::FindObject(json, MaterialKey, report);
    if (object == nullptr)
    {
        return;
    }

    ModelJson::ItemModelValueReader reader(*object, MaterialKey, report);
    ReadAdditiveMeshes(reader, model.material);
    ReadAnimations(reader, model.material, report);
    ReadLayers(reader, model.material, report);
    double specular = 0.0;
    if (reader.ReadNumber(SpecularKey, specular, false))
    {
        if (specular < 0.0 || specular > MaxSpecular)
        {
            reader.Error(SpecularKey, "must be from 0 to " + std::to_string(static_cast<int>(MaxSpecular)));
        }
        else
        {
            model.material.specular = specular;
        }
    }
    reader.WarnAboutUnknownKeys();
}
} // namespace Data::Items::MaterialJson

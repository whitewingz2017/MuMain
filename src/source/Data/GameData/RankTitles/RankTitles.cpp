#include "stdafx.h"
#include "Data/GameData/RankTitles/RankTitles.h"

#include "Data/DataHandler/TextFile.h"
#include "Data/GameData/EffectData/EffectTypeCatalogue.h"

#include "json.hpp"

#include <cmath>
#include <filesystem>
#include <optional>
#include <string_view>
#include <unordered_map>

namespace Data::RankTitles
{
namespace
{
using Json = nlohmann::json;
using Effects::EffectKind;

constexpr int MaximumRank = 65535;
constexpr int MaximumLayers = 16;
constexpr int MaximumCount = 32;
constexpr float MaximumDistance = 2000.f;
constexpr float MaximumSize = 1000.f;
constexpr float MaximumSpin = 3600.f;
constexpr float MinimumPeriod = 100.f;
constexpr float MaximumPeriod = 60000.f;
constexpr float MaximumScale = 20.f;
constexpr int MaximumSubType = 255;

std::unordered_map<int, RankTitle> titles;
std::vector<int> order;
bool loaded = false;

void Warn(const std::string& where, const char* message)
{
    g_ErrorReport.Write(L"> [RankTitles] %hs: %hs: %hs\r\n", RankTitlesFile, where.c_str(), message);
}

float ReadFloat(const Json& object, const char* key, float fallback, float minimum, float maximum,
                const std::string& where)
{
    const auto value = object.find(key);
    if (value == object.end())
        return fallback;
    if (!value->is_number())
    {
        Warn(where + "." + key, "is not a number");
        return fallback;
    }
    const double number = value->get<double>();
    if (!std::isfinite(number) || number < minimum || number > maximum)
    {
        Warn(where + "." + key, "is out of range");
        return fallback;
    }
    return static_cast<float>(number);
}

int ReadInt(const Json& object, const char* key, int fallback, int minimum, int maximum, const std::string& where)
{
    const auto value = object.find(key);
    if (value == object.end())
        return fallback;
    if (!value->is_number_integer() || *value < minimum || *value > maximum)
    {
        Warn(where + "." + key, "is not a whole number in range");
        return fallback;
    }
    return value->get<int>();
}

std::string ReadString(const Json& object, const char* key, const std::string& where)
{
    const auto value = object.find(key);
    if (value == object.end())
        return {};
    if (!value->is_string())
    {
        Warn(where + "." + key, "is not a text");
        return {};
    }
    return value->get<std::string>();
}

Color ReadColor(const Json& object, const std::string& where)
{
    constexpr Color White{1.f, 1.f, 1.f};
    const auto value = object.find("color");
    if (value == object.end())
        return White;
    if (!value->is_array() || value->size() != White.size())
    {
        Warn(where + ".color", "is not [red, green, blue]");
        return White;
    }
    Color color{};
    for (size_t channel = 0; channel < color.size(); ++channel)
    {
        const Json& component = (*value)[channel];
        if (!component.is_number() || component < 0.0 || component > 1.0)
        {
            Warn(where + ".color", "needs numbers from 0 to 1");
            return White;
        }
        color[channel] = component.get<float>();
    }
    return color;
}

std::optional<AuraLayerType> ReadLayerType(const Json& entry, const std::string& where)
{
    static const std::unordered_map<std::string_view, AuraLayerType> Types = {
        {"halo", AuraLayerType::Halo},           {"groundSeal", AuraLayerType::GroundSeal},
        {"risingRings", AuraLayerType::RisingRings}, {"beams", AuraLayerType::Beams},
        {"orbit", AuraLayerType::Orbit},         {"particles", AuraLayerType::Particles},
    };
    const std::string name = ReadString(entry, "type", where);
    const auto found = Types.find(name);
    if (found == Types.end())
    {
        Warn(where + ".type", "is not halo, groundSeal, risingRings, beams, orbit or particles");
        return std::nullopt;
    }
    return found->second;
}

bool UsesTexture(AuraLayerType type)
{
    return type == AuraLayerType::Halo || type == AuraLayerType::GroundSeal || type == AuraLayerType::Orbit;
}

// "sprite": a name of SpriteTypes.json, or "image": a file of its own.
bool ReadTexture(const Json& entry, const std::string& where, AuraTexture& texture)
{
    texture.image = ReadString(entry, "image", where);
    if (!texture.image.empty())
        return true;
    const std::string sprite = ReadString(entry, "sprite", where);
    const auto type = g_EffectTypeCatalogue.FindType(EffectKind::Sprite, sprite);
    if (!type)
    {
        Warn(where + ".sprite", "names no sprite of SpriteTypes.json and there is no image");
        return false;
    }
    texture.sprite = *type;
    return true;
}

bool ReadParticle(const Json& entry, const std::string& where, AuraLayer& layer)
{
    const auto type = g_EffectTypeCatalogue.FindType(EffectKind::Particle, ReadString(entry, "particle", where));
    if (!type)
    {
        Warn(where + ".particle", "names no particle of ParticleTypes.json");
        return false;
    }
    layer.particle = *type;
    layer.subType = ReadInt(entry, "subType", 0, 0, MaximumSubType, where);
    layer.when = ReadString(entry, "when", where) == "moving" ? EmitWhen::Moving : EmitWhen::Always;
    return true;
}

void ReadShape(const Json& entry, const std::string& where, AuraLayer& layer)
{
    const AuraLayer defaults;
    layer.size = ReadFloat(entry, "size", defaults.size, 0.f, MaximumSize, where);
    layer.scale = ReadFloat(entry, "scale", defaults.scale, 0.f, MaximumScale, where);
    layer.height = ReadFloat(entry, "height", defaults.height, -MaximumDistance, MaximumDistance, where);
    layer.depth = ReadFloat(entry, "depth", defaults.depth, -MaximumDistance, MaximumDistance, where);
    layer.radius = ReadFloat(entry, "radius", defaults.radius, 0.f, MaximumDistance, where);
    layer.spread = ReadFloat(entry, "spread", defaults.spread, -MaximumDistance, MaximumDistance, where);
    layer.rise = ReadFloat(entry, "rise", defaults.rise, 0.f, MaximumDistance, where);
    layer.length = ReadFloat(entry, "length", defaults.length, 0.f, MaximumDistance, where);
    layer.width = ReadFloat(entry, "width", defaults.width, 0.f, MaximumSize, where);
    layer.bob = ReadFloat(entry, "bob", defaults.bob, 0.f, MaximumDistance, where);
    layer.count = ReadInt(entry, "count", defaults.count, 1, MaximumCount, where);
}

void ReadMotion(const Json& entry, const std::string& where, AuraLayer& layer)
{
    const AuraLayer defaults;
    layer.spin = ReadFloat(entry, "spin", defaults.spin, -MaximumSpin, MaximumSpin, where);
    layer.period = ReadFloat(entry, "period", defaults.period, MinimumPeriod, MaximumPeriod, where);
    layer.pulse = ReadFloat(entry, "pulse", defaults.pulse, 0.f, MaximumPeriod, where);
    layer.chance = ReadFloat(entry, "chance", defaults.chance, 0.f, 1.f, where);
    layer.blend = ReadString(entry, "blend", where) == "subtract" ? AuraBlend::Subtract : AuraBlend::Add;
}

std::optional<AuraLayer> ReadLayer(const Json& entry, const std::string& where)
{
    if (!entry.is_object())
    {
        Warn(where, "is not an object");
        return std::nullopt;
    }
    const auto type = ReadLayerType(entry, where);
    if (!type)
        return std::nullopt;

    AuraLayer layer;
    layer.type = *type;
    if (UsesTexture(layer.type) && !ReadTexture(entry, where, layer.texture))
        return std::nullopt;
    if (layer.type == AuraLayerType::Particles && !ReadParticle(entry, where, layer))
        return std::nullopt;
    layer.color = ReadColor(entry, where);
    ReadShape(entry, where, layer);
    ReadMotion(entry, where, layer);
    return layer;
}

std::optional<BadgeAnimation> ReadBadgeAnimation(const Json& badge, const std::string& where, const char* key)
{
    const auto value = badge.find(key);
    if (value == badge.end())
        return std::nullopt;
    const std::string field = where + "." + key;
    if (!value->is_object())
    {
        Warn(field, "is not an object");
        return std::nullopt;
    }

    constexpr int MaximumGridSize = 32;
    constexpr int MaximumFrameMs = 60000;
    const int columns = ReadInt(*value, "columns", 0, 1, MaximumGridSize, field);
    const int rows = ReadInt(*value, "rows", 0, 1, MaximumGridSize, field);
    const int frames = ReadInt(*value, "frames", 0, 1, columns * rows, field);
    const int frameMs = ReadInt(*value, "frameMs", 0, 1, MaximumFrameMs, field);
    std::string image = ReadString(*value, "image", field);
    if (image.empty() || frames == 0 || frameMs == 0)
        return std::nullopt;
    return BadgeAnimation{std::move(image), columns, rows, frames, frameMs};
}

std::optional<BadgeEffect> ReadBadgeEffect(const Json& badge, const std::string& where)
{
    auto animation = ReadBadgeAnimation(badge, where, "effect");
    if (!animation)
        return std::nullopt;
    const auto& value = badge["effect"];
    BadgeEffect effect{std::move(*animation)};
    effect.scale = ReadFloat(value, "scale", effect.scale, .5f, 3.f, where + ".effect");
    effect.opacity = ReadFloat(value, "opacity", effect.opacity, 0.f, 1.f, where + ".effect");
    return effect;
}

Badge ReadBadge(const Json& rank, const std::string& where)
{
    Badge badge;
    const auto value = rank.find("badge");
    if (value == rank.end())
        return badge;
    if (!value->is_object())
    {
        Warn(where + ".badge", "is not an object");
        return badge;
    }
    const std::string badgeWhere = where + ".badge";
    badge.image = ReadString(*value, "image", badgeWhere);
    badge.width = ReadFloat(*value, "width", 0.f, 0.f, MaximumSize, badgeWhere);
    badge.height = ReadFloat(*value, "height", 0.f, 0.f, MaximumSize, badgeWhere);
    badge.offsetX = ReadFloat(*value, "offsetX", 0.f, -MaximumSize, MaximumSize, badgeWhere);
    badge.offsetY = ReadFloat(*value, "offsetY", 0.f, -MaximumSize, MaximumSize, badgeWhere);
    badge.animation = ReadBadgeAnimation(*value, badgeWhere, "animation");
    badge.effect = ReadBadgeEffect(*value, badgeWhere);
    if (badge.width <= 0.f || badge.height <= 0.f)
        badge.image.clear();
    return badge;
}

std::vector<AuraLayer> ReadAura(const Json& rank, const std::string& where)
{
    std::vector<AuraLayer> aura;
    const auto value = rank.find("aura");
    if (value == rank.end())
        return aura;
    if (!value->is_array() || value->size() > MaximumLayers)
    {
        Warn(where + ".aura", "is not a list of at most 16 layers");
        return aura;
    }
    for (size_t index = 0; index < value->size(); ++index)
    {
        if (auto layer = ReadLayer((*value)[index], where + ".aura[" + std::to_string(index) + "]"))
            aura.push_back(std::move(*layer));
    }
    return aura;
}

void ReadRank(const Json& entry, size_t index)
{
    const std::string where = "ranks[" + std::to_string(index) + "]";
    if (!entry.is_object())
    {
        Warn(where, "is not an object");
        return;
    }
    RankTitle title;
    title.rank = ReadInt(entry, "rank", 0, 1, MaximumRank, where);
    if (title.rank == 0 || titles.contains(title.rank))
    {
        Warn(where + ".rank", "is missing or used twice");
        return;
    }
    title.badge = ReadBadge(entry, where);
    title.aura = ReadAura(entry, where);
    order.push_back(title.rank);
    titles.emplace(title.rank, std::move(title));
}

void Load()
{
    const auto text = ReadTextFile(std::filesystem::path(RankTitlesFile));
    if (!text)
    {
        Warn("file", "could not be read");
        return;
    }
    const Json json = Json::parse(*text, nullptr, false);
    if (json.is_discarded() || !json.is_object() || ReadInt(json, "formatVersion", 0, 1, FormatVersion, "file") == 0)
    {
        Warn("file", "is not JSON of formatVersion 1");
        return;
    }
    const auto ranks = json.find("ranks");
    if (ranks == json.end() || !ranks->is_array())
    {
        Warn("ranks", "is not a list");
        return;
    }
    for (size_t index = 0; index < ranks->size(); ++index)
        ReadRank((*ranks)[index], index);
}

bool CatalogueReady()
{
    return g_EffectTypeCatalogue.GetTypeCount(EffectKind::Sprite) > 0;
}

// The names in the file resolve through the catalogue, which the loading screen builds.
bool EnsureLoaded()
{
    if (loaded)
        return true;
    if (!CatalogueReady())
        return false;
    loaded = true;
    Load();
    return true;
}
} // namespace

const RankTitle* Find(int rank)
{
    if (rank <= 0 || !EnsureLoaded())
        return nullptr;
    const auto found = titles.find(rank);
    return found != titles.end() ? &found->second : nullptr;
}

const std::vector<int>& Ranks()
{
    EnsureLoaded();
    return order;
}
} // namespace Data::RankTitles

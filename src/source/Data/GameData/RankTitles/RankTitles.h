#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

// The rank titles of Data/Custom/RankTitles/RankTitles.json: the badge over the name and the aura
// around the body of each rank the server gives a player (docs/rank-titles.md).
namespace Data::RankTitles
{
inline constexpr const char* RankTitlesFile = "Data/Custom/RankTitles/RankTitles.json";
constexpr int FormatVersion = 1;

using Color = std::array<float, 3>;

enum class AuraLayerType
{
    Halo,
    GroundSeal,
    RisingRings,
    Beams,
    Orbit,
    Particles,
};

enum class AuraBlend
{
    Add,
    Subtract,
};

// When a particles layer emits.
enum class EmitWhen
{
    Always,
    Moving,
};

// A texture a layer draws with: a sprite of the effect catalogue, or an image file of its own.
struct AuraTexture
{
    int sprite = -1;
    std::string image;
};

// One part of an aura. Which fields a type reads is listed in docs/rank-titles.md.
struct AuraLayer
{
    AuraLayerType type = AuraLayerType::Halo;
    AuraTexture texture;
    int particle = -1;
    int subType = 0;
    AuraBlend blend = AuraBlend::Add;
    EmitWhen when = EmitWhen::Always;
    Color color{1.f, 1.f, 1.f};
    float size = 150.f;
    float scale = 1.f;
    float height = 0.f;
    float depth = 0.f;
    float radius = 70.f;
    float spread = 0.f;
    float rise = 200.f;
    float length = 60.f;
    float width = 2.f;
    float spin = 0.f;
    float bob = 0.f;
    float period = 2000.f;
    float pulse = 0.f;
    float chance = 1.f;
    int count = 4;
};

struct BadgeAnimation
{
    std::string image;
    int columns;
    int rows;
    int frames;
    int frameMs;
};

struct BadgeEffect
{
    BadgeAnimation animation;
    float scale = 1.25f;
    float opacity = .7f;
};

// The badge drawn centred over the name, in interface units.
struct Badge
{
    std::string image;
    float width = 0.f;
    float height = 0.f;
    float offsetX = 0.f;
    float offsetY = 0.f;
    std::optional<BadgeAnimation> animation;
    std::optional<BadgeEffect> effect;
};

struct RankTitle
{
    int rank = 0;
    Badge badge;
    std::vector<AuraLayer> aura;
};

// The title of a rank, nullptr for rank 0 and ranks the file does not have. The file is read on the
// first call after the effect catalogue is built; a file that cannot be used is logged once and
// leaves every rank without a title.
const RankTitle* Find(int rank);

// Every rank the file has, in the file's order; empty until the file can be read (see Find).
const std::vector<int>& Ranks();
} // namespace Data::RankTitles

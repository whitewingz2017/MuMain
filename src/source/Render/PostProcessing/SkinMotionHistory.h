#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

namespace Render::PostProcessing
{

// Where a skinned draw puts its vertices, apart from its bones: its transform (model-view-projection,
// without the temporal jitter) and how the skinning places and scales the body
// (mu::SkinningParameters).
struct SkinPlacement
{
    glm::mat4 transform{1.0f};
    glm::vec3 bodyOrigin{0.0f};
    float bodyScale = 1.0f;
    float boneScale = 1.0f;
    float restPoseScale = 0.0f;
    bool translate = false;

    bool operator==(const SkinPlacement&) const = default;
};

// How a draw's palette was posed and placed in the previous frame.
struct PreviousSkin
{
    // 12 floats a bone, at least as many bones as the draw uses.
    std::span<const float> bones;
    SkinPlacement placement;
};

// The poses bone palettes were drawn in this frame and the frame before, so temporal anti-aliasing
// can tell how far a character's surface moved. A palette is known by its address: a character's
// own bones stay at one address from frame to frame. An address several objects pose into in turn
// (the shared bones of map objects) holds more than one pose in a frame, or one pose at more places
// than an object draws itself at: it has no motion. One pose may be drawn at a few placements (a
// larger outline, copies of a companion); each draw finds the placement the previous frame drew in
// the same turn.
class SkinMotionHistory
{
public:
    // Records that this frame draws the palette at `key` (contents `version`, as
    // mu::SkinningParameters::paletteVersion) posed as `bones` and placed at `placement`. Returns how
    // it was posed and placed in the previous frame: nothing when it was not drawn then, held more
    // than one pose then, had fewer bones then than `bones`, or was not drawn at as many placements.
    // The bones stay valid until BeginFrame.
    [[nodiscard]] std::optional<PreviousSkin> Record(const void* key, std::uint32_t version,
                                                     std::span<const float> bones, const SkinPlacement& placement);

    // Starts a frame: the poses recorded so far become the previous frame's.
    void BeginFrame();

    // Forgets every pose: the next frame has no motion.
    void Reset();

private:
    struct Entry
    {
        std::vector<float> bones;
        // The placements drawn, in the order first drawn.
        std::vector<SkinPlacement> placements;
        std::uint32_t version = 0u;
        // Posed more than one way this frame.
        bool shared = false;
    };

    // Whether `bones` are the pose `entry` holds, as far as both have bones; the longer bones are kept.
    [[nodiscard]] static bool MatchesPose(Entry& entry, std::span<const float> bones);
    // The turn `placement` takes among `entry`'s placements, added when new.
    [[nodiscard]] static std::size_t PlacementTurn(Entry& entry, const SkinPlacement& placement);

    std::unordered_map<const void*, Entry> m_current;
    std::unordered_map<const void*, Entry> m_previous;
};

} // namespace Render::PostProcessing

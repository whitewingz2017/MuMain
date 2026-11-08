// BMD vertex motion for temporal anti-aliasing: one bone per vertex.

// The skinned vertex layout; glslang would otherwise number these 0, 1 (dxc ignores vk::location).
struct VSInput
{
    [[vk::location(0)]] float3 pos : TEXCOORD0;
    [[vk::location(4)]] int positionBoneIndex : TEXCOORD4;
};

#include "skinned_motion.hlsli"

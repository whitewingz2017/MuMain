// How far a skinned draw's surface moved since the previous frame, for temporal anti-aliasing
// (object_motion.frag writes it): each vertex skinned in this frame's pose and in the previous
// frame's, each through its own transform. Shared by skinned_motion.vert (BMD vertices) and
// skinned_weighted_motion.vert (glTF vertices); the including file declares VSInput first, and
// defines MU_WEIGHTED_SKINNING for the weighted one. The skinning is skinned_vertex.hlsli's, for a
// pose given as an argument.
#ifdef MU_D3D12_RAW_STORAGE_BUFFER
ByteAddressBuffer boneRows : register(t0, space0);
static const uint BoneRowSizeBytes = 16;
#else
StructuredBuffer<float4> boneRows : register(t0, space0);
#endif

// Same layout as skinned_textured.vert.hlsl; only the transform and the placement are read.
cbuffer Skinning : register(b0, space1)
{
    float4x4 mvp; // with this frame's jitter, as the scene drew it
    float4 bodyOriginAndScale;
    float4 boneScaleAndRestPoseScale;
    uint4 palette;
};

// Mirrors mu::SkinMotionUniforms.
cbuffer Motion : register(b1, space1)
{
    float4x4 currentTransform;  // mvp without the jitter
    float4x4 previousTransform; // the draw's transform in the previous frame
    float4 previousBodyOriginAndScale;
    float4 previousBoneScaleAndRestPoseScale;
    uint4 previousPalette;
};

struct VSOutput
{
    float4 pos : SV_Position;
    float4 currentClip : TEXCOORD0;
    float4 previousClip : TEXCOORD1;
    float clipDist : SV_ClipDistance0;
};

// Where a pose's bones are and how it places the body.
struct SkinPose
{
    float4 bodyOriginAndScale;
    float4 boneScaleAndRestPoseScale;
    uint4 palette;
};

// Moves the drawn depth this much toward the camera (in clip space, times w): the scene drew the
// surface from another shader, whose depth may round the other way; anything in front of it still
// hides it.
static const float kDepthBias = 2.0e-6;

// These two come first: spirv-cross numbers MSL buffers in order of first use, and SDL wants the
// uniform buffers (Skinning, then Motion) ahead of the storage buffer.
SkinPose CurrentPose()
{
    SkinPose pose;
    pose.bodyOriginAndScale = bodyOriginAndScale;
    pose.boneScaleAndRestPoseScale = boneScaleAndRestPoseScale;
    pose.palette = palette;
    return pose;
}

SkinPose PreviousPose()
{
    SkinPose pose;
    pose.bodyOriginAndScale = previousBodyOriginAndScale;
    pose.boneScaleAndRestPoseScale = previousBoneScaleAndRestPoseScale;
    pose.palette = previousPalette;
    return pose;
}

#ifdef MU_D3D12_RAW_STORAGE_BUFFER
float4 loadBoneRow(uint row)
{
    return asfloat(boneRows.Load4(row * BoneRowSizeBytes));
}
#else
float4 loadBoneRow(uint row)
{
    return boneRows[row];
}
#endif

// skinned_vertex.hlsli's placeBody().
float3 placeBody(float3 transformed, SkinPose pose)
{
    if (pose.palette.z != 0)
    {
        transformed = pose.bodyOriginAndScale.xyz + transformed * pose.bodyOriginAndScale.w;
    }
    return transformed;
}

// skinned_vertex.hlsli's transformPosition().
float3 transformPosition(float3 position, int boneIndex, SkinPose pose)
{
    if (boneIndex < 0 || boneIndex >= int(pose.palette.y))
    {
        return position;
    }

    const uint row = pose.palette.x + uint(boneIndex) * 3;
    const float4 row0 = loadBoneRow(row);
    const float4 row1 = loadBoneRow(row + 1);
    const float4 row2 = loadBoneRow(row + 2);
    float3 transformed;

    if (pose.boneScaleAndRestPoseScale.x == 1.0)
    {
        const float restScale = pose.boneScaleAndRestPoseScale.y != 0.0 ? pose.boneScaleAndRestPoseScale.y : 1.0;
        const float4 restPosition = float4(position * restScale, 1.0);
        transformed = float3(dot(row0, restPosition), dot(row1, restPosition), dot(row2, restPosition));
    }
    else
    {
        transformed = float3(dot(row0.xyz, position), dot(row1.xyz, position), dot(row2.xyz, position));
        transformed = transformed * pose.boneScaleAndRestPoseScale.x + float3(row0.w, row1.w, row2.w);
    }

    return placeBody(transformed, pose);
}

#ifdef MU_WEIGHTED_SKINNING
// skinned_vertex.hlsli's skinWeightedPosition(): the palette holds each joint's matrix times its
// inverse bind matrix.
float3 skinWeightedPosition(float3 position, uint4 joints, float4 weights, SkinPose pose)
{
    const float4 bindPosition = float4(position, 1.0);
    float3 transformed = float3(0.0, 0.0, 0.0);
    [unroll]
    for (uint k = 0; k < 4; ++k)
    {
        if (weights[k] > 0.0 && joints[k] < pose.palette.y)
        {
            const uint row = pose.palette.x + joints[k] * 3;
            transformed += weights[k] * float3(dot(loadBoneRow(row), bindPosition),
                                               dot(loadBoneRow(row + 1), bindPosition),
                                               dot(loadBoneRow(row + 2), bindPosition));
        }
    }
    return placeBody(transformed, pose);
}
#endif

float3 skinnedPosition(VSInput input, SkinPose pose)
{
#ifdef MU_WEIGHTED_SKINNING
    if (input.weights.x > 0.0)
    {
        return skinWeightedPosition(input.pos, input.joints, input.weights, pose);
    }
#endif
    return transformPosition(input.pos, input.positionBoneIndex, pose);
}

VSOutput main(VSInput input)
{
    const float4 current = float4(skinnedPosition(input, CurrentPose()), 1.0);
    const float4 previous = float4(skinnedPosition(input, PreviousPose()), 1.0);

    VSOutput output;
    output.pos = mul(mvp, current);
    output.pos.z -= kDepthBias * output.pos.w;
    output.currentClip = mul(currentTransform, current);
    output.previousClip = mul(previousTransform, previous);
    output.clipDist = output.pos.w - 1.0;
    return output;
}

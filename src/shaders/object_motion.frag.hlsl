// object_motion.frag.hlsl — how far each pixel of a skinned draw moved since the previous frame, in
// clip space, for temporal anti-aliasing (post_temporal.frag): the previous position minus the
// current one, both without the jitter. Alpha 1 marks the pixels it wrote; elsewhere the camera's
// movement says how the scene moved.
struct FSInput
{
    float4 pos : SV_Position;
    float4 currentClip : TEXCOORD0;
    float4 previousClip : TEXCOORD1;
};

// A position this close to the camera's plane (or behind it) is not on screen.
static const float kMinimumClipW = 1.0e-5;
// A movement post_temporal.frag takes for off screen: it then keeps no history for the pixel.
static const float kOffScreenMovement = 1.0e4;

float4 main(FSInput input) : SV_Target
{
    if (input.currentClip.w <= kMinimumClipW || input.previousClip.w <= kMinimumClipW)
    {
        return float4(kOffScreenMovement, kOffScreenMovement, 0.0, 1.0);
    }
    const float2 movement =
        input.previousClip.xy / input.previousClip.w - input.currentClip.xy / input.currentClip.w;
    return float4(movement, 0.0, 1.0);
}

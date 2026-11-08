// post_temporal.frag.hlsl — temporal anti-aliasing: this frame's 3D scene, drawn at a sub-pixel
// offset, blended with the result of the previous frames where the same surface was then: where
// skinned draws moved it (object_motion.frag), or else where the camera's movement puts it. The
// result is the next frame's history and the scene the composite processes.
// TemporalUniforms mirrors mu::SdlGpuTemporalUniform; Render::PostProcessing::TemporalJitter.h has
// the CPU side of the jitter and the reprojection.
Texture2D scene : register(t0, space2);
SamplerState sceneSampler : register(s0, space2);     // point: one scene texel per pixel
Texture2D sceneDepth : register(t1, space2);
SamplerState depthSampler : register(s1, space2);     // point
Texture2D objectMotion : register(t2, space2);        // object_motion.frag's; alpha 1 where it wrote
SamplerState objectMotionSampler : register(s2, space2); // point
Texture2D history : register(t3, space2);
SamplerState historySampler : register(s3, space2);   // linear: the history is read between texels
cbuffer TemporalUniforms : register(b0, space3)
{
    float4x4 currentToPrevious; // this frame's unjittered clip space to the previous frame's
    float2 inverseTargetSize;
    float2 jitter;              // the scene's offset this frame, in clip space
    float historyWeight;        // the previous frames' share; 0: no history
    float objectMotionWritten;  // 1: objectMotion holds this frame's skinned draws
    float padding0;             // scalars: the same offsets under HLSL and SPIR-V packing
    float padding1;
};
struct FSInput
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};

// The colours the history may take: the current pixel's neighbourhood in YCoCg, where a box fits
// the colours of one surface more tightly than in RGB.
struct ColorBox
{
    float3 minimum;
    float3 maximum;
};

// How many standard deviations around the neighbourhood's mean colour the box reaches. Lower
// rejects more stale history (less ghosting behind moving objects) and lets more aliasing through.
static const float kVarianceClipGamma = 1.0;
static const float kNeighbourhoodSize = 9.0;
// Keeps a flat neighbourhood's box from collapsing to a point.
static const float kClipEpsilon = 1.0e-4;
// A previous position this close to the previous camera's plane (or behind it) is not on screen.
static const float kMinimumClipW = 1.0e-5;
static const float kFarDepth = 1.0;
// A skinned surface that moved further than this in one frame (in clip space, which spans 2) keeps
// no history: an animation that jumped, or a pose taken for another object's.
static const float kMaximumObjectMovement = 0.25;

// Defined after main so the compiled shader declares its textures in binding order (scene, depth,
// object motion, history): spirv-cross numbers Metal textures in that order.
ColorBox NeighbourhoodBox(float2 uv, float3 centre);
float2 HistoryUv(float2 uv);
float3 SampleHistory(float2 uv);
float3 ClipToBox(float3 color, ColorBox box);
float3 RgbToYCoCg(float3 rgb);
float3 YCoCgToRgb(float3 ycocg);

float4 main(FSInput input) : SV_Target
{
    const float2 uv = input.pos.xy * inverseTargetSize;
    const float4 current = scene.Sample(sceneSampler, uv);
    if (historyWeight <= 0.0)
    {
        return current;
    }

    const ColorBox box = NeighbourhoodBox(uv, RgbToYCoCg(current.rgb));
    const float2 historyUv = HistoryUv(uv);
    if (any(historyUv < 0.0) || any(historyUv > 1.0))
    {
        return current; // the surface was off screen
    }

    const float3 previous = YCoCgToRgb(ClipToBox(RgbToYCoCg(SampleHistory(historyUv)), box));
    // Reprojected texture detail needs fresher samples while the surface moves across pixels.
    const float motionPixels = length((historyUv - uv) / inverseTargetSize);
    const float blendWeight = historyWeight / (1.0 + motionPixels);
    return float4(lerp(current.rgb, previous, blendWeight), current.a);
}

ColorBox NeighbourhoodBox(float2 uv, float3 centre)
{
    float3 minimum = centre;
    float3 maximum = centre;
    float3 sum = centre;
    float3 sumOfSquares = centre * centre;
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            if (x == 0 && y == 0)
            {
                continue;
            }
            const float2 neighbourUv = uv + float2(x, y) * inverseTargetSize;
            const float3 neighbour = RgbToYCoCg(scene.Sample(sceneSampler, neighbourUv).rgb);
            minimum = min(minimum, neighbour);
            maximum = max(maximum, neighbour);
            sum += neighbour;
            sumOfSquares += neighbour * neighbour;
        }
    }

    // The tighter of the neighbourhood's range and its spread around the mean (variance clipping):
    // one outlying neighbour does not widen the box much.
    const float3 mean = sum / kNeighbourhoodSize;
    const float3 deviation = sqrt(max(sumOfSquares / kNeighbourhoodSize - mean * mean, 0.0));
    ColorBox box;
    box.minimum = max(minimum, mean - kVarianceClipGamma * deviation);
    box.maximum = min(maximum, mean + kVarianceClipGamma * deviation);
    return box;
}

float2 UvToClip(float2 uv)
{
    return float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
}

float2 ClipToUv(float2 clip)
{
    return float2(clip.x * 0.5 + 0.5, 0.5 - clip.y * 0.5);
}

// Where this pixel's surface was in the previous frame's history: the pixel moved as far as the
// nearest surface around it (so a thin near edge carries its own movement, not the background's):
// as its skinned draw moved it, or else as the camera's movement moved it. Outside [0, 1] when it
// was not on screen.
float2 HistoryUv(float2 uv)
{
    // Depth is 0 at the near plane and 1 at the far one (and where nothing was drawn).
    float closestDepth = kFarDepth;
    float2 closestUv = uv;
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            const float2 neighbourUv = uv + float2(x, y) * inverseTargetSize;
            const float depth = sceneDepth.Sample(depthSampler, neighbourUv).r;
            if (depth < closestDepth)
            {
                closestDepth = depth;
                closestUv = neighbourUv;
            }
        }
    }

    if (objectMotionWritten > 0.0)
    {
        const float4 motion = objectMotion.Sample(objectMotionSampler, closestUv);
        if (motion.a > 0.0)
        {
            if (any(abs(motion.xy) > kMaximumObjectMovement))
            {
                return float2(-1.0, -1.0);
            }
            return ClipToUv(UvToClip(uv) + motion.xy);
        }
    }

    // The surface's position without this frame's jitter, so a still camera finds the same pixel.
    const float2 clip = UvToClip(closestUv) - jitter;
    const float4 previousClip = mul(currentToPrevious, float4(clip, closestDepth, 1.0));
    if (previousClip.w <= kMinimumClipW)
    {
        return float2(-1.0, -1.0);
    }
    const float2 movement = previousClip.xy / previousClip.w - clip;
    return ClipToUv(UvToClip(uv) + movement);
}

// The history at `uv` through a Catmull-Rom filter: sharper than one bilinear read, so the history
// does not soften each frame it is reprojected. Five bilinear reads stand in for the 4x4 texels.
float3 SampleHistory(float2 uv)
{
    const float2 position = uv / inverseTargetSize;
    const float2 centre = floor(position - 0.5) + 0.5;
    const float2 f = position - centre;

    const float2 weight0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    const float2 weight1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    const float2 weight2 = f * (0.5 + f * (2.0 - 1.5 * f));
    const float2 weight3 = f * f * (-0.5 + 0.5 * f);
    const float2 weight12 = weight1 + weight2;

    const float2 uv0 = (centre - 1.0) * inverseTargetSize;
    const float2 uv12 = (centre + weight2 / weight12) * inverseTargetSize;
    const float2 uv3 = (centre + 2.0) * inverseTargetSize;

    float3 color = history.SampleLevel(historySampler, float2(uv12.x, uv0.y), 0.0).rgb * (weight12.x * weight0.y);
    color += history.SampleLevel(historySampler, float2(uv0.x, uv12.y), 0.0).rgb * (weight0.x * weight12.y);
    color += history.SampleLevel(historySampler, uv12, 0.0).rgb * (weight12.x * weight12.y);
    color += history.SampleLevel(historySampler, float2(uv3.x, uv12.y), 0.0).rgb * (weight3.x * weight12.y);
    color += history.SampleLevel(historySampler, float2(uv12.x, uv3.y), 0.0).rgb * (weight12.x * weight3.y);
    // The corner texels are left out: the weights read do not quite sum to 1.
    const float weightSum = weight12.x * weight0.y + weight0.x * weight12.y + weight12.x * weight12.y +
                            weight3.x * weight12.y + weight12.x * weight3.y;
    return max(color / weightSum, 0.0);
}

// `color` pulled toward the box's centre until it lies inside the box.
float3 ClipToBox(float3 color, ColorBox box)
{
    const float3 centre = 0.5 * (box.maximum + box.minimum);
    const float3 extent = 0.5 * (box.maximum - box.minimum) + kClipEpsilon;
    const float3 offset = color - centre;
    const float3 units = abs(offset / extent);
    const float largest = max(units.x, max(units.y, units.z));
    return largest > 1.0 ? centre + offset / largest : color;
}

float3 RgbToYCoCg(float3 rgb)
{
    return float3(0.25 * rgb.r + 0.5 * rgb.g + 0.25 * rgb.b, 0.5 * rgb.r - 0.5 * rgb.b,
                  -0.25 * rgb.r + 0.5 * rgb.g - 0.25 * rgb.b);
}

float3 YCoCgToRgb(float3 ycocg)
{
    return float3(ycocg.x + ycocg.y - ycocg.z, ycocg.x + ycocg.z, ycocg.x - ycocg.y - ycocg.z);
}

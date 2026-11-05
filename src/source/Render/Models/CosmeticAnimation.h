#pragma once

struct _PART_t;
class CHARACTER;
namespace Render::Models::CosmeticAnimation
{
// Steps a cosmetic equipment part (weapon or wing skin) on its own clock; false for any other part.
bool AnimateCosmeticPart(CHARACTER& character, _PART_t& part, int modelType);
// Applies a TwinMU item model's equipment offset, rotation and scale to the part's parent matrix.
void AdjustAttachment(int modelType, float parent[3][4]);
}

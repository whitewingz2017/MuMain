#pragma once

struct _PART_t;
class CHARACTER;
namespace Render::Models::CosmeticAnimation
{
void AnimateAttachment(CHARACTER& character, _PART_t& part, int modelType, bool linked,
                       float position[3], float angle[3], float parent[3][4]);
void AdjustAttachment(int modelType, float parent[3][4]);
}

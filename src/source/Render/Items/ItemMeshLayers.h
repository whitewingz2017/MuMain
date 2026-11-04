#pragma once

class BMD;
class OBJECT;

namespace Render::Items
{
bool RenderMeshLayers(BMD* model, OBJECT* object, int type, float alpha, int flags);
}

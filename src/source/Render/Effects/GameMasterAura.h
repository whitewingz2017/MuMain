#pragma once

class CHARACTER;
class OBJECT;

namespace Render::Effects::GameMasterAura
{
void RenderBehind(const CHARACTER* character, const OBJECT* object);
void Render(const CHARACTER* character, const OBJECT* object);
}

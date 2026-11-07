#include "stdafx.h"
#include "UI/Chat/RankBadge.h"

#include "Camera/CameraProjection.h"
#include "Camera/CameraState.h"
#include "Character/CSParts.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Data/GameData/RankTitles/RankTitles.h"
#include "GameLogic/RankTitles/RankTitleStore.h"
#include "Render/Textures/ImageByPath.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "UI/Chat/Chat.h"

#include <SDL3/SDL_timer.h>

#include <algorithm>
#include <limits>

namespace UI::RankTitles::Badge
{
namespace
{
// The space between the badge and the top of the name box.
constexpr float NameBoxGap = 2.f;
constexpr float HeadClearance = 60.f;
constexpr float VisibleAlpha = .01f;

int GameMasterMarkTop(const CHARACTER& character, int idleTop)
{
    if (character.EtcPart != PARTS_WEBZEN || !character.m_pParts)
        return idleTop;
    const OBJECT* mark = static_cast<CSIPartsMDL*>(character.m_pParts)->GetObject();
    if (mark->Type != MODEL_WEBZEN_MARK || mark->Alpha <= VisibleAlpha)
        return idleTop;
    const auto& bounds = mark->OBB;
    if (bounds.XAxis[0] == 0.f && bounds.YAxis[1] == 0.f && bounds.ZAxis[2] == 0.f)
        return idleTop;
    int markTop = std::numeric_limits<int>::max();
    for (int corner = 0; corner < 8; ++corner)
    {
        vec3_t position;
        for (int axis = 0; axis < 3; ++axis)
        {
            position[axis] = bounds.StartPos[axis] + ((corner & 1) ? bounds.XAxis[axis] : 0.f)
                + ((corner & 2) ? bounds.YAxis[axis] : 0.f) + ((corner & 4) ? bounds.ZAxis[axis] : 0.f);
        }
        int x = 0;
        int y = 0;
        CameraProjection::WorldToScreen(g_Camera, position, &x, &y);
        markTop = std::min(markTop, y);
    }
    return markTop;
}

bool RenderAnimation(const ::Data::RankTitles::BadgeAnimation& animation, float left, float top,
                     float width, float height, float opacity)
{
    if (opacity <= 0.f)
        return false;
    const GLuint texture = ::Render::Textures::ImageByPath::Get(animation.image);
    if (texture == BITMAP_UNKNOWN)
        return false;
    const int frame = static_cast<int>((SDL_GetTicks() / animation.frameMs) % animation.frames);
    const float tileWidth = 1.f / animation.columns;
    const float tileHeight = 1.f / animation.rows;
    EnableAlphaTest();
    RenderBitmap(static_cast<int>(texture), left, top, width, height,
                 (frame % animation.columns) * tileWidth, (frame / animation.columns) * tileHeight,
                 tileWidth, tileHeight, true, true, opacity);
    return true;
}

void RenderEffect(const ::Data::RankTitles::Badge& badge, float left, float top)
{
    if (!badge.effect)
        return;
    const auto& effect = *badge.effect;
    const float width = badge.width * effect.scale;
    const float height = badge.height * effect.scale;
    RenderAnimation(effect.animation, left - (width - badge.width) / 2.f,
                    top - (height - badge.height) / 2.f, width, height, effect.opacity);
}
} // namespace

void Render(const CHARACTER* character, int x, int y, int width)
{
    const auto* title = ::Data::RankTitles::Find(::GameLogic::RankTitles::RankOf(character));
    if (!title || title->badge.image.empty())
        return;
    const auto& badge = title->badge;
    const float left = x + width / 2.f - badge.width / 2.f + badge.offsetX;
    const float effectOverhang = badge.effect && badge.effect->opacity > 0.f
        ? std::max(0.f, (badge.effect->scale - 1.f) * badge.height / 2.f) : 0.f;
    const float top = y - badge.height - NameBoxGap - effectOverhang + badge.offsetY;
    RenderEffect(badge, left, top);
    if (badge.animation && RenderAnimation(*badge.animation, left, top, badge.width, badge.height, 1.f))
        return;
    const GLuint texture = ::Render::Textures::ImageByPath::Get(badge.image);
    if (texture == BITMAP_UNKNOWN)
        return;
    EnableAlphaTest();
    RenderBitmap(static_cast<int>(texture), left, top, badge.width, badge.height);
}
void RenderVisible()
{
    for (int index = 0; index < MAX_CHARACTERS_CLIENT; ++index)
    {
        CHARACTER& character = CharactersClient[index];
        OBJECT& object = character.Object;
        if (!object.Live || !object.Visible || object.Kind != KIND_PLAYER || object.Type != MODEL_PLAYER
            || object.Alpha <= VisibleAlpha || ::GameLogic::RankTitles::RankOf(&character) == 0)
            continue;
        if (g_isCharacterBuff((&object), eBuff_Cloaking) || g_isCharacterBuff((&object), eBuff_CrywolfNPCHide))
            continue;
        vec3_t position;
        Vector(object.Position[0], object.Position[1], object.Position[2] + object.BoundingBoxMax[2] + HeadClearance,
               position);
        int x = 0;
        int y = 0;
        CameraProjection::WorldToScreen(g_Camera, position, &x, &y);
        y = UI::Chat::NameLabelTop(&character, GameMasterMarkTop(character, y));
        Render(&character, x, y, 0);
    }
}
} // namespace UI::RankTitles::Badge

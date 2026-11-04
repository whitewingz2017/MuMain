///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Render/Textures/ZzzOpenglUtil.h"
#include "Render/Models/ZzzBMD.h"
#include "Engine/Object/ZzzInfomation.h"
#include "Engine/Object/ZzzObject.h"
#include "Engine/Object/ZzzCharacter.h"
#include "Render/Terrain/ZzzLodTerrain.h"
#include "Render/Textures/ZzzTexture.h"
#include "Engine/AI/ZzzAI.h"
#include "ZzzEffect.h"
#include "Audio/DSPlaySound.h"
#include "Network/Server/WSclient.h"
#include "UI/Core/WindowSystem.h"
#include "Scenes/MainScene.h"
#include "Render/Renderer/MuRenderer.h"
#include "Camera/CameraProjection.h"
#include "Render/Items/ScopedItemEffects.h"

#include <algorithm>
#include <array>
#include <bitset>
#include <cstring>
#include <optional>

namespace
{
constexpr float kSpriteFrameMin = 0.2f;
constexpr float kSpriteFrameMax = 1.0f;
constexpr float kSpriteFrameStep = 0.1f;

float ComputeFrameDelta()
{
    return kSpriteFrameStep * FPS_ANIMATION_FACTOR;
}

float UpdateAnimationFrame(float currentFrame, bool isVisible)
{
    const float delta = ComputeFrameDelta();
    currentFrame += isVisible ? delta : -delta;
    return std::clamp(currentFrame, kSpriteFrameMin, kSpriteFrameMax);
}
} // namespace

OBJECT Sprites[MAX_SPRITES];

namespace
{
// Sprites created inside a bloom exclusion - a character's gem sparkles, item
// glows and eye lights - are drawn inside one, so post-processing leaves the
// character's Classic look alone.
std::array<bool, MAX_SPRITES> s_spriteBloomExcluded{};
} // namespace

int CreateSprite(int Type, vec3_t Position, float Scale, vec3_t Light, OBJECT* Owner, float Rotation, int SubType)
{
    if (!g_pOption->GetRenderAllEffects())
    {
        return false;
    }

    for (int i = 0; i < MAX_SPRITES; i++)
    {
        OBJECT* o = &Sprites[i];
        if (!o->Live)
        {
            o->Live = true;
            o->Type = Type;
            o->SubType = SubType;
            o->Owner = Owner;
            s_spriteBloomExcluded[i] = mu::GetRenderer().IsBloomExcluded();
            o->AnimationFrame = 1.f;
            o->Scale = Scale;
            o->Angle[2] = Rotation;
            VectorCopy(Position, o->Position);
            VectorCopy(Position, o->StartPosition);
            VectorCopy(Light, o->Light);
            return i;
        }
    }
    return false;
}

void RenderSprite(OBJECT* o, OBJECT* Owner)
{
    o->AnimationFrame = UpdateAnimationFrame(o->AnimationFrame, o->Visible);
    float Scale = o->AnimationFrame * o->Scale;

    BITMAP_t* pBitmap = Bitmaps.GetTexture(o->Type);
    float Width = pBitmap->Width * Scale;
    float Height = pBitmap->Height * Scale;

    if (o->Type == BITMAP_FORMATION_MARK)
    {
        float u = 0.0f, v = 0.0f, uw, vw;
        uw = 0.33f;
        vw = 0.33f;
        switch (o->SubType)
        {
        case 0:
            u = 0.f;
            v = 0.f;
            break;

        case 1:
            u = 0.33f;
            v = 0.f;
            break;

        case 2:
            u = 0.66f;
            v = 0.f;
            break;

        case 3:
            u = 0.f;
            v = 0.33f;
            break;

        case 4:
            u = 0.33f;
            v = 0.33f;
            break;

        case 5:
            u = 0.66f;
            v = 0.33f;
            break;

        case 6:
            u = 0.f;
            v = 0.66f;
            break;

        case 7:
            u = 0.33f;
            v = 0.66f;
            break;
        }

        RenderSprite(o->Type, o->Position, 64, 64, o->Light, o->Angle[2], u, v, uw, vw);
    }
    else
    {
        RenderSprite(o->Type, o->Position, Width, Height, o->Light, o->Angle[2]);
    }
}

namespace
{
void RenderSpriteWithBlend(int index)
{
    auto* sprite = &Sprites[index];
    if (sprite->Type == BITMAP_FORMATION_MARK || sprite->SubType == 2) EnableAlphaTest();
    else if (sprite->SubType == 0) EnableAlphaBlend();
    else if (sprite->SubType == 1) EnableAlphaBlendMinus();
    else if (sprite->SubType == 3) EnableAlphaBlend2();
    std::optional<mu::ScopedBloomExclusion> exclusion;
    if (s_spriteBloomExcluded[index]) exclusion.emplace();
    RenderSprite(sprite, sprite->Owner);
}
}

void RenderWithSpriteCamera(const std::function<void()>& draw)
{
    float savedMatrix[3][4];
    std::memcpy(savedMatrix, g_Camera.Matrix, sizeof(savedMatrix));
    CameraProjection::GetOpenGLMatrix(g_Camera.Matrix);
    BeginSprite();
    draw();
    EndSprite();
    std::memcpy(g_Camera.Matrix, savedMatrix, sizeof(savedMatrix));
}

void RenderWithSprites(const std::function<void()>& draw)
{
    // Preview attachments must stay in this camera and out of the world particle pool.
    const Render::Items::ScopedItemEffects effects(Render::Items::ScopedItemEffects::Enabled(), true);
    std::bitset<MAX_SPRITES> existing;
    for (int i = 0; i < MAX_SPRITES; ++i) existing[i] = Sprites[i].Live;
    draw();
    float savedMatrix[3][4];
    std::memcpy(savedMatrix, g_Camera.Matrix, sizeof(savedMatrix));
    CameraProjection::GetOpenGLMatrix(g_Camera.Matrix);
    BeginSprite();
    for (int i = 0; i < MAX_SPRITES; ++i)
    {
        auto& sprite = Sprites[i];
        if (existing[i] || !sprite.Live) continue;
        sprite.Visible = true;
        if (!IsSpritesDisabledDebug()) RenderSpriteWithBlend(i);
        sprite.Live = false;
    }
    EndSprite();
    std::memcpy(g_Camera.Matrix, savedMatrix, sizeof(savedMatrix));
}

void RenderSprites(BYTE byRenderOneMore)
{
    if (!g_pOption->GetRenderAllEffects())
    {
        return;
    }
    if (IsSpritesDisabledDebug())
    {
        return;
    }

    for (int i = 0; i < MAX_SPRITES; i++)
    {
        OBJECT* o = &Sprites[i];
        if (byRenderOneMore == 1)
        {
            if (o->Position[2] > 350.f)
            {
                continue;
            }
        }
        else if (byRenderOneMore == 2)
        {
            if (o->Position[2] <= 300.f)
            {
                o->Live = false;
                continue;
            }
        }

        if (o->Live)
        {
            RenderSpriteWithBlend(i);

            if (byRenderOneMore == 0 || byRenderOneMore == 2)
            {
                o->Live = false;
            }
        }
    }
}

void CheckSprites()
{
    if (!g_pOption->GetRenderAllEffects())
    {
        return;
    }

    for (int i = 0; i < MAX_SPRITES; i++)
    {
        OBJECT* o = &Sprites[i];
        if (o->Live)
        {
            o->Visible = true;
        }
    }
}

#include "stdafx.h"
#include "Render/Textures/ImageByPath.h"

#include "Core/Utilities/StringUtils.h"
#include "Render/Sprites/GlobalBitmap.h"

#include <unordered_map>
#include <unordered_set>

namespace Render::Textures::ImageByPath
{
namespace
{
// Unpadded, so a badge of any size fills its texture and draws whole with the full texture
// coordinates; RmlUi draws its unpadded images through the same renderer.
constexpr bool PadToPowerOfTwo = false;
constexpr bool PremultiplyAlpha = false;

std::unordered_map<std::string, GLuint> loaded;
std::unordered_set<std::string> failed;
} // namespace

GLuint Get(const std::string& path)
{
    if (path.empty() || failed.contains(path))
        return BITMAP_UNKNOWN;
    const auto found = loaded.find(path);
    if (found != loaded.end() && Bitmaps.FindTexture(found->second))
        return found->second;

    // Its own entry, so a numbered slot that another scene reloads cannot swap the image under the cache.
    const GLuint texture = Bitmaps.LoadImageExclusive(StringUtils::NarrowToWide(path), GL_LINEAR, GL_CLAMP_TO_EDGE,
                                                      PadToPowerOfTwo, PremultiplyAlpha);
    if (texture == BITMAP_UNKNOWN)
    {
        failed.insert(path);
        g_ErrorReport.Write(L"> [ImageByPath] %hs could not be loaded.\r\n", path.c_str());
        return BITMAP_UNKNOWN;
    }
    loaded[path] = texture;
    return texture;
}
} // namespace Render::Textures::ImageByPath

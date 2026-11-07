#pragma once

#include <string>

// Images that data files name by path (Data/Custom/RankTitles/RankTitles.json): loaded on first use,
// and again when the texture cache dropped them.
namespace Render::Textures::ImageByPath
{
// The texture of the image at `path`, relative to the client folder; BITMAP_UNKNOWN when it cannot be
// loaded. A file that fails is logged once and not tried again.
GLuint Get(const std::string& path);
} // namespace Render::Textures::ImageByPath

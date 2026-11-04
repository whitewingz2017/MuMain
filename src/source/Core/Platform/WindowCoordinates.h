#pragma once

#include <SDL3/SDL.h>

namespace Core::Platform
{
inline bool GameWindowSize(SDL_Window* window, int* width, int* height)
{
    return SDL_GetWindowSize(window, width, height);
}

inline float WindowToGameScale(SDL_Window* window)
{
    (void)window;
    return 1.f;
}

inline void MouseEventToGameCoordinates(SDL_Event& event)
{
    (void)event;
}
}

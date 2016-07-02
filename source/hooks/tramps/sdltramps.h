/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#include "../intercept.h"

#define SDLCALL __cdecl

namespace Hooks
{
    struct SDL_Palette {
        int entryCount;
        PALETTEENTRY* entries;
    };
    struct SDL_PixelFormat {
        SDL_Palette* palette;
        unsigned char BitsPerPixel;
        unsigned char BytesPerPixel;
        unsigned char Rloss;
        unsigned char Gloss;
        unsigned char Bloss;
        unsigned char Aloss;
        unsigned char Rshift;
        unsigned char Gshift;
        unsigned char Bshift;
        unsigned char Ashift;
        unsigned int Rmask;
        unsigned int Gmask;
        unsigned int Bmask;
        unsigned int Amask;
        unsigned int colorkey;
        unsigned char alpha;
    };
    struct SDL_Surface
    {
        unsigned int flags;
        SDL_PixelFormat* format;
        int w;
        int h;
        unsigned short pitch;
        void* pixels;
    };
    struct SDL_Rect
    {
        signed short x, y;
        unsigned short w, h;
    };

    HOOK_DECLARE(int, SDLCALL, SDL_Flip, SDL_Surface *screen);
    HOOK_DECLARE(void, SDLCALL, SDL_UpdateRect, SDL_Surface *screen, int x, int y, int w, int h);
    HOOK_DECLARE(void, SDLCALL, SDL_UpdateRects, SDL_Surface* screen, int numrects, SDL_Rect* rects);
    HOOK_DECLARE(void, SDLCALL, SDL_GL_SwapBuffers);
    HOOK_DECLARE(SDL_Surface*, SDLCALL, SDL_SetVideoMode, int width, int height, int bpp, unsigned int flags);
    HOOK_DECLARE(int, SDLCALL, SDL_LockSurface, SDL_Surface *surface);
    HOOK_DECLARE(void, SDLCALL, SDL_UnlockSurface, SDL_Surface *surface);

    void ApplySDLIntercepts();
}

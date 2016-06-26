/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

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

#define SDLCALL __cdecl

#define SDL_Flip TrampSDL_Flip
TRAMPFUNC int SDLCALL SDL_Flip(SDL_Surface *screen);
#define SDL_UpdateRect TrampSDL_UpdateRect
TRAMPFUNC void SDLCALL SDL_UpdateRect(SDL_Surface *screen, int x, int y, int w, int h);
#define SDL_UpdateRects TrampSDL_UpdateRects
TRAMPFUNC void SDLCALL SDL_UpdateRects(SDL_Surface* screen, int numrects, SDL_Rect* rects);
#define SDL_GL_SwapBuffers TrampSDL_GL_SwapBuffers
TRAMPFUNC void SDLCALL SDL_GL_SwapBuffers();
#define SDL_SetVideoMode TrampSDL_SetVideoMode
TRAMPFUNC SDL_Surface* SDLCALL SDL_SetVideoMode(int width, int height, int bpp, unsigned int flags);
#define SDL_LockSurface TrampSDL_LockSurface
TRAMPFUNC int SDLCALL SDL_LockSurface (SDL_Surface *surface);
#define SDL_UnlockSurface TrampSDL_UnlockSurface
TRAMPFUNC void SDLCALL SDL_UnlockSurface (SDL_Surface *surface);

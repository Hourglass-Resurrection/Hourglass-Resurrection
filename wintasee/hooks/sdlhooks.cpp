/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

// hooking SDL at all should be unnecessary,
// but a lot of what we need to do requires guessing the program's "intent",
// and SDL's APIs make more of the programmer's intent directly obvious
// than do the lower-level APIs.
// there's little reason not to take advantage of that.

#if !defined(SDLHOOKS_INCL) && !defined(UNITY_BUILD)
#define SDLHOOKS_INCL

#include "../global.h"
#include "../../shared/ipc.h"
#include "../wintasee.h"

bool PresentOGLD3D(); // extern

static bool used_sdl_flip = false;

static void SDLFrameBoundary(SDL_Surface* screen)
{
	// are we (not) recording AVI?
	BOOL recordingAVIVideo = (tasflags.aviMode & 1);
	if(!recordingAVIVideo || !screen)
	{
		// if not recording AVI, it's a regular frame boundary.
		FrameBoundary(NULL, CAPTUREINFO_TYPE_NONE);
	}
	else
	{
		// if we are, it's still a regular frame boundary,
		// but we prepare extra info for the AVI capture around it.
		DDSURFACEDESC desc = { sizeof(DDSURFACEDESC) };
		SDL_LockSurface(screen);
		desc.lpSurface = screen->pixels;
		desc.dwWidth = screen->w;
		desc.dwHeight = screen->h;
		desc.lPitch = screen->pitch;
		desc.ddpfPixelFormat.dwRGBBitCount = screen->format->BitsPerPixel;
		desc.ddpfPixelFormat.dwRBitMask = screen->format->Rmask;
		desc.ddpfPixelFormat.dwGBitMask = screen->format->Gmask;
		desc.ddpfPixelFormat.dwBBitMask = screen->format->Bmask;
		if(desc.ddpfPixelFormat.dwRGBBitCount == 8 && screen->format->palette)
			memcpy(&activePalette[0], &screen->format->palette->entries[0], min(256, screen->format->palette->entryCount));

		FrameBoundary(&desc, CAPTUREINFO_TYPE_DDSD);

		SDL_UnlockSurface(screen); 
	}
}

HOOKFUNC int SDLCALL MySDL_Flip(SDL_Surface* screen)
{
	debuglog(LCF_SDL|LCF_FRAME, __FUNCTION__ " called.\n");
	used_sdl_flip = true;
	usingSDLOrDD = true;
	int prevFrameCount = framecount;
	int rv;
	if(!ShouldSkipDrawing(true, false))
		rv = SDL_Flip(screen);
	else
		rv = 0;
	if(prevFrameCount == framecount)
		SDLFrameBoundary(screen);
	return rv;
}
HOOKFUNC void SDLCALL MySDL_UpdateRect(SDL_Surface* screen, int x, int y, int w, int h)
{
	usingSDLOrDD = true;
	bool isEntireScreen = !x && !y && !w && !h;
	debuglog(LCF_SDL|(isEntireScreen?LCF_FRAME:0), __FUNCTION__ "(0x%X, %d,%d,%d,%d) called.\n", screen, x,y,w,h);
	if(!ShouldSkipDrawing(isEntireScreen, /*!isEntireScreen*/false)) // eversion crashes sometimes if the second param is true
		SDL_UpdateRect(screen, x,y,w,h);
	// used_sdl_flip check needed for eversion, not needed for ?
	if(!used_sdl_flip && isEntireScreen)
		SDLFrameBoundary(screen);
}
HOOKFUNC void SDLCALL MySDL_UpdateRects(SDL_Surface* screen, int numrects, SDL_Rect* rects)
{
	debuglog(LCF_SDL|LCF_FRAME, __FUNCTION__ "(0x%X, %d, 0x%X) called.\n", screen, numrects, rects);
	usingSDLOrDD = true;
	if(!ShouldSkipDrawing(true, true))
		SDL_UpdateRects(screen, numrects, rects);
	// used_sdl_flip check needed for eversion, not needed for ?
	if(!used_sdl_flip)
		SDLFrameBoundary(screen);
}
HOOKFUNC void SDLCALL MySDL_GL_SwapBuffers()
{
	debuglog(LCF_SDL|LCF_OGL|LCF_FRAME, __FUNCTION__ " called.\n");
	usingSDLOrDD = true;
	bool alreadyDidBoundary = false;
	//if(!ShouldSkipDrawing(true, false))
	{
		//SDL_GL_SwapBuffers(); // FIXME: replace with directx equivalent since opengl is lethal when combined with savestates
		if(PresentOGLD3D())
			alreadyDidBoundary = true;
	}
	if(!used_sdl_flip && !alreadyDidBoundary)
		FrameBoundary();
}
HOOKFUNC SDL_Surface* SDLCALL MySDL_SetVideoMode(int width, int height, int bpp, unsigned int flags)
{
	debuglog(LCF_SDL|LCF_UNTESTED, __FUNCTION__ "(%d,%d,%d,0x%X) called.\n",width,height,bpp,flags);
	static const int SDL_FULLSCREEN = 0x80000000;
	static const int SDL_HWACCEL = 0x00000100;
	if(tasflags.forceWindowed)
		flags &= ~SDL_FULLSCREEN;
	if(tasflags.forceSoftware)
		flags &= ~SDL_HWACCEL;
	SDL_Surface* rv = SDL_SetVideoMode(width, height, bpp, flags);
	return rv;
}

HOOKFUNC int MySDL_LockSurface (SDL_Surface *surface)
{
	debuglog(LCF_SDL|LCF_FREQUENT, __FUNCTION__ "(0x%X) called.\n", surface);
	return SDL_LockSurface(surface);
}
HOOKFUNC void MySDL_UnlockSurface (SDL_Surface *surface)
{
	debuglog(LCF_SDL|LCF_FREQUENT, __FUNCTION__ "(0x%X) called.\n", surface);
	SDL_UnlockSurface(surface);
}


void ApplySDLIntercepts()
{
	static const InterceptDescriptor intercepts [] = 
	{
		MAKE_INTERCEPT(1, SDL, SDL_Flip),
		MAKE_INTERCEPT(1, SDL, SDL_UpdateRect),
		MAKE_INTERCEPT(1, SDL, SDL_UpdateRects),
		MAKE_INTERCEPT(1, SDL, SDL_GL_SwapBuffers),
		MAKE_INTERCEPT(1, SDL, SDL_SetVideoMode), // not necessary, but maybe helps app behave more nicely
		MAKE_INTERCEPT(0, SDL, SDL_LockSurface),
		MAKE_INTERCEPT(0, SDL, SDL_UnlockSurface),
	};
	ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
}


#else
#pragma message(__FILE__": (skipped compilation)")
#endif

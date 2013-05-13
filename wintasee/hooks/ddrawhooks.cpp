/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(DDRAWHOOKS_INCL) && !defined(UNITY_BUILD)
#define DDRAWHOOKS_INCL

#include "../../external/ddraw.h"
#include "../global.h"
#include "../wintasee.h"
#include "../tls.h"
#include <map>

#include "../phasedetection.h"
static PhaseDetector s_phaseDetector;

void FakeBroadcastDisplayChange(int width, int height, int depth);

DEFINE_LOCAL_GUID(IID_IDirectDraw,  0x6C14DB80,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60);
DEFINE_LOCAL_GUID(IID_IDirectDraw2, 0xB3A6F3E0,0x2B43,0x11CF,0xA2,0xDE,0x00,0xAA,0x00,0xB9,0x33,0x56);
DEFINE_LOCAL_GUID(IID_IDirectDraw4, 0x9c59509a,0x39bd,0x11d1,0x8c,0x4a,0x00,0xc0,0x4f,0xd9,0x30,0xc5);
DEFINE_LOCAL_GUID(IID_IDirectDraw7, 0x15e65ec0,0x3b9c,0x11d2,0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b);

template<typename IDirectDrawN> struct IDirectDrawTraits {};
template<> struct IDirectDrawTraits<IDirectDraw>    { typedef IDirectDrawSurface  DIRECTDRAWSURFACEN; typedef DDSURFACEDESC  DDSURFACEDESCN; typedef DDSCAPS  DDSCAPSN; typedef DDDEVICEIDENTIFIER  DDDEVICEIDENTIFIERN; enum{NUMBER=1}; };
template<> struct IDirectDrawTraits<IDirectDraw2>   { typedef IDirectDrawSurface  DIRECTDRAWSURFACEN; typedef DDSURFACEDESC  DDSURFACEDESCN; typedef DDSCAPS  DDSCAPSN; typedef DDDEVICEIDENTIFIER  DDDEVICEIDENTIFIERN; enum{NUMBER=2}; };
template<> struct IDirectDrawTraits<IDirectDraw4>   { typedef IDirectDrawSurface4 DIRECTDRAWSURFACEN; typedef DDSURFACEDESC2 DDSURFACEDESCN; typedef DDSCAPS2 DDSCAPSN; typedef DDDEVICEIDENTIFIER  DDDEVICEIDENTIFIERN; enum{NUMBER=4}; };
template<> struct IDirectDrawTraits<IDirectDraw7>   { typedef IDirectDrawSurface7 DIRECTDRAWSURFACEN; typedef DDSURFACEDESC2 DDSURFACEDESCN; typedef DDSCAPS2 DDSCAPSN; typedef DDDEVICEIDENTIFIER2 DDDEVICEIDENTIFIERN; enum{NUMBER=7}; };

static int s_numMyDirectDraws = 0;

BOOL fakeDisplayValid = FALSE;
int fakeDisplayWidth = 0;
int fakeDisplayHeight = 0;
/*static*/ int fakePixelFormatBPP = 0;
/*static*/ int fakeDisplayRefresh = 0;


static LPRECT a_bltsaved = 0;
static LPDIRECTDRAWSURFACE b_bltsaved = 0;
static LPRECT c_bltsaved = 0;
static DWORD d_bltsaved = 0;
static LPDDBLTFX e_bltsaved = 0;
static LPDIRECTDRAWSURFACE dds_bltsaved = 0;
static RECT a_bltsaved_buf = {0};
static RECT c_bltsaved_buf = {0};
static DDBLTFX e_bltsaved_buf = {0};

struct ManualHDCInfo
{
	HBITMAP oldBitmap;
};
std::map<HDC,ManualHDCInfo> manuallyCreatedSurfaceDCs;


void RescaleRect(RECT& rect, RECT from, RECT to);
void ConfineRect(RECT& rect, RECT bounds)
{
	RECT tempRect = rect;
	if(tempRect.left < bounds.left)
	{
		tempRect.right += bounds.left - tempRect.left;
		tempRect.left = bounds.left;
		if(tempRect.right > bounds.right)
			tempRect.right = bounds.right;
	}
	else if(tempRect.right > bounds.right)
	{
		tempRect.left += bounds.right - tempRect.right;
		tempRect.right = bounds.right;
		if(tempRect.left < bounds.left)
			tempRect.left = bounds.left;
	}
	if(tempRect.top < bounds.top)
	{
		tempRect.bottom += bounds.top - tempRect.top;
		tempRect.top = bounds.top;
		if(tempRect.bottom > bounds.bottom)
			tempRect.bottom = bounds.bottom;
	}
	else if(tempRect.bottom > bounds.bottom)
	{
		tempRect.top += bounds.bottom - tempRect.bottom;
		tempRect.bottom = bounds.bottom;
		if(tempRect.top < bounds.top)
			tempRect.top = bounds.top;
	}
	rect = tempRect;
}
//LPRECT ExpandRect(RECT& rect, RECT other)
//{
//	RECT src = rect;
//	rect.left = min(src.left, other.left);
//	rect.top = min(src.top, other.top);
//	rect.right = max(src.right, other.right);
//	rect.bottom = max(src.bottom, other.bottom);
//	return &rect;
//}



DEFINE_LOCAL_GUID(IID_IDirectDrawSurface,  0x6C14DB81,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60);
DEFINE_LOCAL_GUID(IID_IDirectDrawSurface2, 0x57805885,0x6EEC,0x11CF,0x94,0x41,0xA8,0x23,0x03,0xC1,0x0E,0x27);
DEFINE_LOCAL_GUID(IID_IDirectDrawSurface3, 0xDA044E00,0x69B2,0x11D0,0xA1,0xD5,0x00,0xAA,0x00,0xB8,0xDF,0xBB);
DEFINE_LOCAL_GUID(IID_IDirectDrawSurface4, 0x0B2B8630,0xAD35,0x11D0,0x8E,0xA6,0x00,0x60,0x97,0x97,0xEA,0x5B);
DEFINE_LOCAL_GUID(IID_IDirectDrawSurface7, 0x06675A80,0x3B9B,0x11D2,0xB9,0x2F,0x00,0x60,0x97,0x97,0xEA,0x5B);



template<typename IDirectDrawSurfaceN> struct IDirectDrawSurfaceTraits {};
template<> struct IDirectDrawSurfaceTraits<IDirectDrawSurface>  { typedef IDirectDrawSurface  DIRECTDRAWSURFACEN; typedef DDSURFACEDESC  DDSURFACEDESCN; typedef DDSCAPS  DDSCAPSN; typedef LPVOID UNLOCKARG; typedef HRESULT (FAR PASCAL * LPDDENUMSURFACESCALLBACKN)(LPDIRECTDRAWSURFACE, LPDDSURFACEDESC, LPVOID); enum{NUMBER=1}; };
template<> struct IDirectDrawSurfaceTraits<IDirectDrawSurface2> { typedef IDirectDrawSurface2 DIRECTDRAWSURFACEN; typedef DDSURFACEDESC  DDSURFACEDESCN; typedef DDSCAPS  DDSCAPSN; typedef LPVOID UNLOCKARG; typedef HRESULT (FAR PASCAL * LPDDENUMSURFACESCALLBACKN)(LPDIRECTDRAWSURFACE, LPDDSURFACEDESC, LPVOID); enum{NUMBER=2}; };
template<> struct IDirectDrawSurfaceTraits<IDirectDrawSurface3> { typedef IDirectDrawSurface3 DIRECTDRAWSURFACEN; typedef DDSURFACEDESC  DDSURFACEDESCN; typedef DDSCAPS  DDSCAPSN; typedef LPVOID UNLOCKARG; typedef HRESULT (FAR PASCAL * LPDDENUMSURFACESCALLBACKN)(LPDIRECTDRAWSURFACE, LPDDSURFACEDESC, LPVOID); enum{NUMBER=3}; };
template<> struct IDirectDrawSurfaceTraits<IDirectDrawSurface4> { typedef IDirectDrawSurface4 DIRECTDRAWSURFACEN; typedef DDSURFACEDESC2 DDSURFACEDESCN; typedef DDSCAPS2 DDSCAPSN; typedef LPRECT UNLOCKARG; typedef HRESULT (FAR PASCAL * LPDDENUMSURFACESCALLBACKN)(LPDIRECTDRAWSURFACE4, LPDDSURFACEDESC2, LPVOID); enum{NUMBER=4}; };
template<> struct IDirectDrawSurfaceTraits<IDirectDrawSurface7> { typedef IDirectDrawSurface7 DIRECTDRAWSURFACEN; typedef DDSURFACEDESC2 DDSURFACEDESCN; typedef DDSCAPS2 DDSCAPSN; typedef LPRECT UNLOCKARG; typedef HRESULT (FAR PASCAL * LPDDENUMSURFACESCALLBACKN)(LPDIRECTDRAWSURFACE7, LPDDSURFACEDESC2, LPVOID); enum{NUMBER=7}; };

static void* s_hookAttachPass = NULL;
static void* s_theBackBuffer = NULL;

//static void* pDDBackBufCopyOwner = NULL;
//static void* pDDBackBufCopy = NULL;

struct IDirectDrawOwnerInfo
{
	void* ddrawInterface;
	int ddrawVersionNumber;
};
static std::map<void*,IDirectDrawOwnerInfo> s_ddrawSurfaceToOwner;


// instead of inheriting from IDirectDrawSurface and returning one as a wrapper of the original,
// I keep the same object type and use vtable hooking to point its functions at my functions.
// (so the following class is never instantiated, it's all pseudo-object-oriented static functions.)
// I do this because plenty of functions expect a real IDirectDrawSurface passed into them,
// and will crash if I give them something else even if it implements the interface completely.
// it's possible to perform the conversion to/from real IDirectDrawSurfaces at each such crash site
// but it's much easier to get it right by simply leaving the objects alone as much as possible,
// plus it has the benefit that I don't have to type 50 zillion functions I don't need to change.
template <typename IDirectDrawSurfaceN>
struct MyDirectDrawSurface
{
	typedef typename IDirectDrawSurfaceTraits<IDirectDrawSurfaceN>::DIRECTDRAWSURFACEN DIRECTDRAWSURFACEN;
	typedef typename IDirectDrawSurfaceTraits<IDirectDrawSurfaceN>::DDSURFACEDESCN DDSURFACEDESCN;
	typedef typename IDirectDrawSurfaceTraits<IDirectDrawSurfaceN>::DDSCAPSN DDSCAPSN;
	typedef typename IDirectDrawSurfaceTraits<IDirectDrawSurfaceN>::LPDDENUMSURFACESCALLBACKN LPDDENUMSURFACESCALLBACKN;
	typedef typename IDirectDrawSurfaceTraits<IDirectDrawSurfaceN>::UNLOCKARG UNLOCKARG;

	static BOOL Hook(DIRECTDRAWSURFACEN* obj)
	{
		BOOL rv = FALSE;
		rv |= VTHOOKFUNC(DIRECTDRAWSURFACEN, Blt);
		rv |= VTHOOKFUNC(DIRECTDRAWSURFACEN, BltFast);
		rv |= VTHOOKFUNC(DIRECTDRAWSURFACEN, BltBatch);
		rv |= VTHOOKFUNC(DIRECTDRAWSURFACEN, Release);

		rv |= VTHOOKFUNC(DIRECTDRAWSURFACEN, GetSurfaceDesc);
		rv |= VTHOOKFUNC(DIRECTDRAWSURFACEN, GetPixelFormat);
		rv |= VTHOOKFUNC(DIRECTDRAWSURFACEN, GetCaps);
		rv |= VTHOOKFUNC(DIRECTDRAWSURFACEN, Flip);
		rv |= VTHOOKFUNC(DIRECTDRAWSURFACEN, Lock);
		rv |= VTHOOKFUNC(DIRECTDRAWSURFACEN, Unlock);
		rv |= VTHOOKFUNC(DIRECTDRAWSURFACEN, GetDC);
		rv |= VTHOOKFUNC(DIRECTDRAWSURFACEN, ReleaseDC);
		rv |= VTHOOKFUNC(DIRECTDRAWSURFACEN, GetFlipStatus);
		rv |= VTHOOKFUNC(DIRECTDRAWSURFACEN, SetPalette);
		rv |= VTHOOKFUNC(DIRECTDRAWSURFACEN, SetColorKey);
		rv |= VTHOOKFUNC(DIRECTDRAWSURFACEN, GetAttachedSurface);

		//rv |= VTHOOKFUNC(DIRECTDRAWSURFACEN, AddAttachedSurface);
		//rv |= VTHOOKFUNC(DIRECTDRAWSURFACEN, GetBltStatus);
		//rv |= VTHOOKFUNC(DIRECTDRAWSURFACEN, DeleteAttachedSurface);
		//rv |= VTHOOKFUNC(DIRECTDRAWSURFACEN, EnumAttachedSurfaces);
		//rv |= VTHOOKFUNC(DIRECTDRAWSURFACEN, EnumOverlayZOrders);
		//rv |= VTHOOKFUNC(DIRECTDRAWSURFACEN, GetClipper);
		//rv |= VTHOOKFUNC(DIRECTDRAWSURFACEN, IsLost);

		rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");

		if(s_hookAttachPass)
		{
			ddrawdebugprintf("copy attached: 0x%X -> 0x%X\n", obj, s_hookAttachPass);
			attachedSurfaces[obj] = (DIRECTDRAWSURFACEN*)s_hookAttachPass; // TODO: wrong type, does it matter?
		}

		videoMemoryBackupDirty[obj] = TRUE;

		//if(std::find(surfaceList.begin(), surfaceList.end(), obj) == surfaceList.end())
		//	surfaceList.push_back(obj);

		return rv;
	}

	static ULONG (STDMETHODCALLTYPE *Release)(DIRECTDRAWSURFACEN* pThis);
	static ULONG STDMETHODCALLTYPE MyRelease(DIRECTDRAWSURFACEN* pThis)
	{
		ULONG rv = Release(pThis);
		ddrawdebugprintf(__FUNCTION__ "(0x%X) called, refcount -> %d.\n", pThis, rv);
		//cmdprintf("SHORTTRACE: 3,50");
		// note: pThis is invalid to dereference now, if rv==0
		if((void*)b_bltsaved == (void*)pThis)
			b_bltsaved = 0;
		if((void*)dds_bltsaved == (void*)pThis)
			dds_bltsaved = 0;
		//if(!rv)
		//	surfaceList.erase(std::remove(surfaceList.begin(), surfaceList.end(), pThis), surfaceList.end());
		if(!rv)
		{
			if((void*)s_theBackBuffer == (void*)pThis)
				s_theBackBuffer = 0;

			videoMemoryBackupDirty[pThis] = FALSE;

			void*& pixels = videoMemoryPixelBackup[pThis];
			free(pixels);
			pixels = NULL;
		}
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(DIRECTDRAWSURFACEN* pThis, REFIID riid, void** ppvObj);
	static HRESULT STDMETHODCALLTYPE MyQueryInterface(DIRECTDRAWSURFACEN* pThis, REFIID riid, void** ppvObj)
	{
		ddrawdebugprintf(__FUNCTION__ " called (0x%X).\n", riid.Data1);
		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
		if(SUCCEEDED(rv))
		{
			s_hookAttachPass = attachedSurfaces[pThis];
			HookCOMInterface(riid, ppvObj);
			s_hookAttachPass = NULL;
		}
		return rv;
	}

	static void HandleNewFrame(int xOrigin, int yOrigin, DIRECTDRAWSURFACEN* pThis, DIRECTDRAWSURFACEN* pSource, RECT* destRect, RECT* srcRect)
	{
		usingSDLOrDD = true;

		// check to eliminate most spurious frames of dirty-region using games
		if(s_phaseDetector.AdvanceAndCheckCycleBoundary(MAKELONG(xOrigin,yOrigin)))
		{
			// it's a new frame, so do frame boundary calculations

			// first save enough data for RedrawScreen to work when the state is loaded
			//static int lastFrameXOrigin=-1, lastFrameYOrigin=-1;
			//if(lastFrameXOrigin != xOrigin || lastFrameYOrigin != yOrigin)
			//{
			//	lastFrameXOrigin = xOrigin;
			//	lastFrameYOrigin = yOrigin;
			//	a_bltsaved_buf = *destRect;
			//}
			//a_bltsaved = destRect ? ExpandRect(a_bltsaved_buf, *destRect) : 0;
			b_bltsaved = (LPDIRECTDRAWSURFACE)pSource;
			//c_bltsaved = srcRect ? ExpandRect(c_bltsaved_buf, *srcRect) : 0;
			//d_bltsaved = 0;//flags;
			//e_bltsaved = 0;//bltfx ? ((e_bltsaved_buf = *bltfx), &e_bltsaved_buf) : 0;
			dds_bltsaved = (LPDIRECTDRAWSURFACE)pThis;

			// are we (not) recording AVI?
			BOOL recordingAVIVideo = (tasflags.aviMode & 1);
			if(!recordingAVIVideo || !pSource)
			{
				// if not recording AVI, it's a regular frame boundary.
				FrameBoundary(NULL, CAPTUREINFO_TYPE_NONE);
			}
			else
			{
				// if we are, it's still a regular frame boundary,
				// but we prepare extra info for the AVI capture around it.
				DDSURFACEDESCN desc = { sizeof(DDSURFACEDESCN) };
#if 0 // slow
				pSource->Lock(NULL, &desc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_READONLY, NULL); 
				FrameBoundary(&desc, CAPTUREINFO_TYPE_DDSD);
				pSource->Unlock(NULL);
#else // MUCH faster. (BltFast to system memory surface instead of locking and reading from video memory)
				DIRECTDRAWSURFACEN* pBackBuffer = pSource;
				DIRECTDRAWSURFACEN* pDDBackBufCopy;
				std::map<IDirectDrawSurfaceN*, IDirectDrawSurfaceN*>::iterator found;
				found = sysMemCopySurfaces.find(pThis);
				if(found != sysMemCopySurfaces.end())
				{
					pDDBackBufCopy = found->second;
				}
				else
				{
					IDirectDrawOwnerInfo info = s_ddrawSurfaceToOwner[pThis];
					if(!info.ddrawInterface)
						info = s_ddrawSurfaceToOwner[pSource]; // needed in fake fullscreen? at least in la-mulana
					pDDBackBufCopy = CreateSysMemSurface(pSource, info);
					sysMemCopySurfaces[pThis] = pDDBackBufCopy;
				}
				DIRECTDRAWSURFACEN* pSurface = pDDBackBufCopy;
				if(pSurface != pBackBuffer)
					if(!pSurface || FAILED(BltFast(pSurface, 0,0,pBackBuffer,NULL,DDBLTFAST_WAIT)))
						pSurface = pBackBuffer; // fallback
				desc.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
				GetPixelFormat(pThis, &desc.ddpfPixelFormat);
				if(desc.ddpfPixelFormat.dwRGBBitCount == 8)
				{
					LPDIRECTDRAWPALETTE pPalette;
					if(SUCCEEDED(pThis->GetPalette(&pPalette)))
					{
						pPalette->GetEntries(0, 0, 256, &activePalette[0]);
						pPalette->Release();
					}
				}
				pSurface->Lock(NULL, &desc, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT | DDLOCK_READONLY, NULL); 
				FrameBoundary(&desc, CAPTUREINFO_TYPE_DDSD);
				pSurface->Unlock(NULL);
#endif
			}
		}
	}

	static HRESULT(STDMETHODCALLTYPE *Blt)(DIRECTDRAWSURFACEN* pThis, LPRECT destRect,DIRECTDRAWSURFACEN* pSource, LPRECT srcRect,DWORD flags, LPDDBLTFX bltfx);
	static HRESULT STDMETHODCALLTYPE MyBlt(DIRECTDRAWSURFACEN* pThis, LPRECT destRect,DIRECTDRAWSURFACEN* pSource, LPRECT srcRect,DWORD flags, LPDDBLTFX bltfx)
	{
		ddrawdebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
		//cmdprintf("SHORTTRACE: 3,50");


		ddrawdebugprintf("(%X,%X,%X,%X,%X).\n", destRect,pSource,srcRect,flags,bltfx);
		if(destRect)
			ddrawdebugprintf("destRect: (%d,%d,%d,%d)\n", destRect->left, destRect->top, destRect->right, destRect->bottom);
		if(srcRect)
			ddrawdebugprintf("srcRect: (%d,%d,%d,%d)\n", srcRect->left, srcRect->top, srcRect->right, srcRect->bottom);

		flags |= DDBLT_WAIT;
		flags &= ~DDBLT_DONOTWAIT;

		DDSCAPSN caps;
		GetCaps(pThis, &caps);
		bool destIsPrimary = (caps.dwCaps & (DDSCAPS_PRIMARYSURFACE|DDSCAPS_FRONTBUFFER)) != 0;

		DDSCAPSN caps2;
		bool srcIsPrimary;
		if(pSource)
		{
			GetCaps(pSource, &caps2);
			srcIsPrimary = (caps2.dwCaps & (DDSCAPS_PRIMARYSURFACE|DDSCAPS_FRONTBUFFER)) != 0;
		}
		else
		{
			srcIsPrimary = false;
		}

		RECT idRect;
		RECT tempSrcRect;
		RECT tempDstRect;

		if(srcIsPrimary || destIsPrimary)
		{
			// might need to set or override the destination rect to front buffer
			// could be because we're in force-windowed mode, or because we're preventing the game from knowing where the window really is

			if(destIsPrimary)
			{
				// we want idrect to be independent of any adjustments we make to the destrect
				if(destRect)
					idRect = *destRect;
				else
					memset(&idRect, 0, sizeof(RECT));
				//POINT pt = {0,0};
				//if(ScreenToClient(gamehwnd, &pt))
				//	OffsetRect(&idRect, pt.x, pt.y);
			}

			RECT realRect;
			if(GetClientRect(gamehwnd, &realRect) && ClientToScreen(gamehwnd, (LPPOINT)&realRect.left) && ClientToScreen(gamehwnd, (LPPOINT)&realRect.right))
			{
				DDSURFACEDESCN ddsdBB = { sizeof(DDSURFACEDESCN) };
				if(!s_theBackBuffer || FAILED(GetSurfaceDesc((DIRECTDRAWSURFACEN*)s_theBackBuffer, &ddsdBB)))
				{
					ddsdBB.dwWidth = fakeDisplayWidth;
					ddsdBB.dwHeight = fakeDisplayHeight;
				}

				if(destIsPrimary)
				{
					if(!destRect)
					{
						// whole screen was target, so fill client
						destRect = &tempDstRect; // avoid altering original rect, since the game owns it
						*destRect = realRect;
					}
					else if(fakeDisplayValid)
					{
						//debugprintf("destRect = %d %d %d %d\n", destRect->left, destRect->top, destRect->right, destRect->bottom);

						// part of screen was target, so rescale to client
						RECT fakeRect = {0, 0, fakeDisplayWidth, fakeDisplayHeight};
						tempDstRect = *destRect; destRect = &tempDstRect; // avoid altering original rect, since the game owns it
						RescaleRect(*destRect, fakeRect, realRect);
						//ConfineRect(*destRect, realRect); // shouldn't need this
					}
					else
					{
						// in this case, we're not currently sure where the game thinks it's rendering to,
						// so the best we can do is push it into view if the game got it wrong.
						// here we make the assumption that the game wants to render into somewhere in its own window's client region.
						tempDstRect = *destRect; destRect = &tempDstRect; // avoid altering original rect, since the game owns it
						ConfineRect(*destRect, realRect);
					}
				}

				if(srcIsPrimary)
				{
					if(!srcRect)
					{
						// whole screen was source, so fill client
						srcRect = &tempSrcRect; // avoid altering original rect, since the game owns it
						*srcRect = realRect;
					}
					else if(fakeDisplayValid)
					{ 
						// part of screen was source, so rescale to client
						RECT fakeRect = {0, 0, fakeDisplayWidth, fakeDisplayHeight};
						tempSrcRect = *srcRect; srcRect = &tempSrcRect; // avoid altering original rect, since the game owns it
						RescaleRect(*srcRect, fakeRect, realRect);
					}
					else
					{
						tempSrcRect = *srcRect; srcRect = &tempSrcRect; // avoid altering original rect, since the game owns it
						ConfineRect(*srcRect, realRect);
					}
				}

				// TODO: if destIsPrimary and source is not the corresponding part of the backbuffer,
				// then this blit won't show up in AVIs.
				// for example, Nova 3000 does frontbuffer-to-frontbuffer blits
				// and incomplete-backbuffer-to-frontbuffer blits for screen transitions.
				// probably need a virtual frontbuffer to support such effects.
			}
		}

		//debugprintf("pThis == 0x%X, pSource == 0x%X\n", pThis, pSource);

		HRESULT rv;
		if(ShouldSkipDrawing(destIsPrimary, pThis == s_theBackBuffer))
			rv = DD_OK;
		else
		{
			//if(srcIsPrimary && srcRect && destRect)
			//{
			//	debugprintf("src = %d %d %d %d  0x%X\n", srcRect->left, srcRect->top, srcRect->right, srcRect->bottom, pSource);
			//	debugprintf("dst = %d %d %d %d  0x%X\n", destRect->left, destRect->top, destRect->right, destRect->bottom, pThis);
			//}


			// do the actual blit
			rv = Blt(pThis, destRect,pSource,srcRect,flags,bltfx);


			if(!destIsPrimary)
				videoMemoryBackupDirty[pThis] = TRUE;
			else if(pSource) // catch the case where the back buffer changes but is never blitted to:
				videoMemoryBackupDirty[pSource] = TRUE;
		}

		if(!srcIsPrimary)
		{
			if(destIsPrimary && pSource)
			{
				if(s_theBackBuffer != pSource)
					ddrawdebugprintf("s_theBackBuffer: 0x%X -> 0x%X\n", s_theBackBuffer, pSource);
				s_theBackBuffer = pSource;
			}

			// if the destination is the front buffer, this is likely a new frame
			if(destIsPrimary && !redrawingScreen)
				HandleNewFrame(idRect.left, idRect.top, pThis, pSource, destRect, srcRect);
		}

		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *BltFast)(DIRECTDRAWSURFACEN* pThis, DWORD xDest,DWORD yDest,DIRECTDRAWSURFACEN* pSource, LPRECT srcRect,DWORD flags);
	static HRESULT STDMETHODCALLTYPE MyBltFast(DIRECTDRAWSURFACEN* pThis, DWORD xDest,DWORD yDest,DIRECTDRAWSURFACEN* pSource, LPRECT srcRect,DWORD flags)
	{
		ddrawdebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);

		// FIXME: probably doesn't handle forced windowed mode correctly (as regular Blt does)
		// I don't know of any games that use it to draw to the fullscreen mode front buffer, though.

		flags |= DDBLTFAST_WAIT;
		flags &= ~DDBLTFAST_DONOTWAIT;

		//ddrawdebugprintf(__FUNCTION__ " called.\n");

		DDSCAPSN caps;
		GetCaps(pThis, &caps);

		bool destIsPrimary = (caps.dwCaps & (DDSCAPS_PRIMARYSURFACE|DDSCAPS_FRONTBUFFER))!=0;

		HRESULT rv;
		if(ShouldSkipDrawing(destIsPrimary, pThis == s_theBackBuffer))
			rv = DD_OK;
		else
		{
			if(!destIsPrimary)
			{
#if 1			// TODO: I can't remember what game this is for, but the code should be updated if not deleted.
				if(pSource)
				{
					DDSCAPSN caps2;
					GetCaps(pSource, &caps2);
					if((caps2.dwCaps & (DDSCAPS_PRIMARYSURFACE|DDSCAPS_FRONTBUFFER))) // srcIsPrimary?
					{
						if(tasflags.forceWindowed && s_theBackBuffer)
						{
							DDSURFACEDESCN ddsdBB = { sizeof(DDSURFACEDESCN) };
							GetSurfaceDesc((DIRECTDRAWSURFACEN*)s_theBackBuffer, &ddsdBB);
							RECT bbRect = {0, 0, ddsdBB.dwWidth, ddsdBB.dwHeight };
							static RECT rect;
							rect = bbRect;
							POINT pt = {0,0};
							if(ClientToScreen(gamehwnd, &pt) && OffsetRect(&rect, pt.x, pt.y))
							{
								if(!srcRect || (fakeDisplayValid && (int)(srcRect->right-xDest) >= fakeDisplayWidth && (int)(srcRect->bottom-yDest) >= fakeDisplayHeight))
								{
									srcRect = &rect;
								}
							}
						}
					}
				}
#endif

				rv = BltFast(pThis, xDest,yDest,pSource,srcRect,flags);
				videoMemoryBackupDirty[pThis] = TRUE;
			}
			else
			{
				// convert BltFast call to a Blt call for draws to the primary buffer.
				// one reason for doing this is tha BltFast doesn't support window clipping,
				// which would be bad in forced-windowed mode.
				if(!srcRect)
				{
					if(xDest == 0 && yDest == 0)
						return MyBlt(pThis, NULL, pSource, NULL, 0, NULL);

					DDSURFACEDESCN ddsdesc = { sizeof(DDSURFACEDESCN) };
					if(pSource)
					{
						if(SUCCEEDED(GetSurfaceDesc(pSource, &ddsdesc)))
						{
							RECT destRect = {xDest, yDest, xDest + ddsdesc.dwWidth, yDest + ddsdesc.dwHeight };
							return MyBlt(pThis, &destRect, pSource, srcRect, 0, NULL);
						}
					}
					if(SUCCEEDED(GetSurfaceDesc(pThis, &ddsdesc)))
					{
						RECT destRect = {xDest, yDest, ddsdesc.dwWidth, ddsdesc.dwHeight };
						return MyBlt(pThis, &destRect, pSource, srcRect, 0, NULL);
					}
					return MyBlt(pThis, NULL, pSource, NULL, 0, NULL);
				}
				RECT destRect = {xDest, yDest, xDest + srcRect->right - srcRect->left, yDest + srcRect->bottom - srcRect->top };
				return MyBlt(pThis, &destRect, pSource, srcRect, 0, NULL);
			}
		}

		//if(destIsPrimary && pSource)
		//{
		//	DDSCAPSN caps2;
		//	GetCaps(pSource, &caps2);
		//	bool srcIsPrimary = (caps2.dwCaps & (DDSCAPS_PRIMARYSURFACE|DDSCAPS_FRONTBUFFER)) != 0;
		//	if(srcIsPrimary)
		//	{
		//		// if the destination and source are both the front buffer, don't treat it as a new frame
		//		destIsPrimary = false;
		//	}
		//	else
		//	{
		//		if(s_theBackBuffer != pSource)
		//			ddrawdebugprintf("s_theBackBuffer: 0x%X -> 0x%X\n", s_theBackBuffer, pSource);
		//		s_theBackBuffer = pSource;
		//	}
		//}

		//// if the destination is the front buffer, this is likely a new frame
		//if(destIsPrimary && !redrawingScreen)
		//	HandleNewFrame(xDest, yDest, pThis, pSource);

		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *BltBatch)(DIRECTDRAWSURFACEN* pThis, LPDDBLTBATCH a, DWORD b, DWORD c);
	static HRESULT STDMETHODCALLTYPE MyBltBatch(DIRECTDRAWSURFACEN* pThis, LPDDBLTBATCH a, DWORD b, DWORD c)
	{
		// NYI... do any games use this?
		ddrawdebugprintf(__FUNCTION__ "(0x%X, 0x%X, 0x%X, 0x%X) called.\n", pThis, a, b, c);
		HRESULT rv = BltBatch(pThis, a, b, c);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *GetCaps)(DIRECTDRAWSURFACEN* pThis, DDSCAPSN* lpddscaps);
	static HRESULT STDMETHODCALLTYPE MyGetCaps(DIRECTDRAWSURFACEN* pThis, DDSCAPSN* lpddscaps)
	{
		HRESULT rv = GetCaps(pThis, lpddscaps);

		ddrawdebugprintf(__FUNCTION__ " called. dwCaps = 0x%X.\n", lpddscaps->dwCaps);

		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *GetSurfaceDesc)(DIRECTDRAWSURFACEN* pThis, DDSURFACEDESCN* lpddsdesc);
	static HRESULT STDMETHODCALLTYPE MyGetSurfaceDesc(DIRECTDRAWSURFACEN* pThis, DDSURFACEDESCN* lpddsdesc)
	{
		HRESULT rv = GetSurfaceDesc(pThis, lpddsdesc);

		ddrawdebugprintf(__FUNCTION__ " called.\n");
		DDSURFACEDESCN& ddsd = *lpddsdesc;
		ddrawdebugprintf("dwFlags = 0x%X.\n", ddsd.dwFlags);
		ddrawdebugprintf("dwWidth,dwHeight = %d,%d.\n", ddsd.dwWidth,ddsd.dwHeight);
		ddrawdebugprintf("lPitch = %d.\n", ddsd.lPitch);
		ddrawdebugprintf("dwBackBufferCount = %d.\n", ddsd.dwBackBufferCount);
		ddrawdebugprintf("dwRefreshRate = %d.\n", ddsd.dwRefreshRate);
		ddrawdebugprintf("lpSurface = %d.\n", ddsd.lpSurface);
		ddrawdebugprintf("ddckCKSrcOverlay = %d.\n", ddsd.ddckCKSrcOverlay);
		ddrawdebugprintf("ddckCKSrcBlt = %d.\n", ddsd.ddckCKSrcBlt);
		ddrawdebugprintf("ddpfPixelFormat.dwRGBBitCount = %d.\n", ddsd.ddpfPixelFormat.dwRGBBitCount);
		ddrawdebugprintf("ddsCaps = 0x%X.\n", ddsd.ddsCaps.dwCaps);
		ddrawdebugprintf("rv = 0x%X.\n", rv);

		//if(fakeDisplayValid)
		//{
		//	//ddsd.ddpfPixelFormat.dwRGBBitCount = fakePixelFormatBPP;
		//	ddsd.dwWidth,ddsd.dwHeight
		//}

		// not sure
		if(ddsd.ddsCaps.dwCaps & DDSCAPS_OFFSCREENPLAIN)
		{
			if(s_ddrawSurfaceToOwner.find(pThis) != s_ddrawSurfaceToOwner.end())
			{
				ddsd.ddsCaps.dwCaps &= ~DDSCAPS_OFFSCREENPLAIN;
				ddsd.ddsCaps.dwCaps |= DDSCAPS_BACKBUFFER | DDSCAPS_COMPLEX | DDSCAPS_FLIP;
				ddrawdebugprintf("ddsCaps = 0x%X.\n", ddsd.ddsCaps.dwCaps);
			}
		}

		// not sure
		if(ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
		{
			if(fakeDisplayValid)
			{
				//ddsd.ddpfPixelFormat.dwRGBBitCount = fakePixelFormatBPP;
				ddsd.dwWidth = fakeDisplayWidth;
				ddsd.dwHeight = fakeDisplayHeight;
				ddrawdebugprintf("dwWidth,dwHeight = %d,%d.\n", ddsd.dwWidth,ddsd.dwHeight);
			}
		}

		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *GetPixelFormat)(DIRECTDRAWSURFACEN* pThis, LPDDPIXELFORMAT lpddpfmt);
	static HRESULT STDMETHODCALLTYPE MyGetPixelFormat(DIRECTDRAWSURFACEN* pThis, LPDDPIXELFORMAT lpddpfmt)
	{
		HRESULT rv = GetPixelFormat(pThis, lpddpfmt);

//#pragma message("FIXMEEE")
//		cmdprintf("DEBUGREDALERT: 0");

		ddrawdebugprintf(__FUNCTION__ " called.\n");
		if(lpddpfmt)
			ddrawdebugprintf("dwRGBBitCount = %d.\n", lpddpfmt->dwRGBBitCount);

		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *Flip)(DIRECTDRAWSURFACEN* pThis, DIRECTDRAWSURFACEN* pOther, DWORD flags);
	static HRESULT STDMETHODCALLTYPE MyFlip(DIRECTDRAWSURFACEN* pThis, DIRECTDRAWSURFACEN* pOther, DWORD flags)
	{
		ddrawdebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);

		HRESULT rv;
		if(!(tasflags.forceWindowed))
		{
			rv = Flip(pThis, pOther, flags);

			videoMemoryBackupDirty[pThis] = TRUE;
			if(pOther)
				videoMemoryBackupDirty[pOther] = TRUE;
			s_theBackBuffer = pThis;

			if(!redrawingScreen)
				HandleNewFrame(0, 0, pOther, pThis, NULL, NULL);
		}
		else
		{
			if(!pOther)
			{
				GetAttachedFakeBackBuf(pThis, &pOther);
			}

			rv = MyBlt(pThis, NULL, pOther, 0, DDBLT_WAIT, 0);
		}

		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *Lock)(DIRECTDRAWSURFACEN* pThis, LPRECT a,DDSURFACEDESCN* b,DWORD lockFlags,HANDLE d);
	static HRESULT STDMETHODCALLTYPE MyLock(DIRECTDRAWSURFACEN* pThis, LPRECT a,DDSURFACEDESCN* b,DWORD lockFlags,HANDLE d)
	{
		HRESULT rv;

		//cmdprintf("SHORTTRACE: 3,50");
		//cmdprintf("DEBUGPAUSE: " __FUNCTION__);

		DDSCAPSN caps;
		GetCaps(pThis, &caps);
		BOOL isPrimary = (caps.dwCaps & (DDSCAPS_PRIMARYSURFACE|DDSCAPS_FRONTBUFFER));

		ddrawdebugprintf(__FUNCTION__ "(0x%X, lockFlags=0x%X, primary=%d) called.\n", pThis, lockFlags, isPrimary);

		lockFlags |= DDLOCK_WAIT;

		if(!isPrimary || IDirectDrawSurfaceTraits<IDirectDrawSurfaceN>::NUMBER > 3) // hack
		{
			rv = Lock(pThis, a,b,lockFlags,d);
		}
		else
		{
			// if possible, lock the backbuffer instead of the front buffer... see MyUnlock()
			DIRECTDRAWSURFACEN* pBackbuffer;
			GetAttachedFakeBackBuf(pThis, &pBackbuffer);
			if(!pBackbuffer)
			{
				rv = Lock(pThis, a,b,lockFlags,d);
			}
			else
			{
#if 1 // temp hack for la-mulana color keys... not sure why but it really needs to read the first onscreen pixel, probably what needs to be fixed is that the game was able to write to that pixel (somehow) earlier and it should have been offset into the window or backbuffer
//cmdprintf("DEBUGPAUSE: " __FUNCTION__);
				if(!(lockFlags & (DDLOCK_WRITEONLY|DDLOCK_DISCARDCONTENTS)))
				{
					//DDSURFACEDESCN ddsdBB = { sizeof(DDSURFACEDESCN), DDSD_WIDTH | DDSD_HEIGHT };
					//GetSurfaceDesc(pBackbuffer, &ddsdBB);
					RECT rect = {0, 0, 1, 1 };
					//RECT rect = {0, 0, ddsdBB.dwWidth, ddsdBB.dwHeight };
					//POINT pt = {0,0};
					//if(ClientToScreen(gamehwnd, &pt))
					//	OffsetRect(&rect, pt.x, pt.y);
					BltFast(pBackbuffer, 0,0,pThis,&rect,DDBLTFAST_WAIT);
				}
#endif
				rv = Lock(pBackbuffer, a,b,lockFlags,d);
			}
		}

//		if(b && fakeDisplayValid)
//			b->ddpfPixelFormat.dwRGBBitCount = fakePixelFormatBPP;

		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *Unlock)(DIRECTDRAWSURFACEN* pThis, UNLOCKARG lockRect);
	static HRESULT STDMETHODCALLTYPE MyUnlock(DIRECTDRAWSURFACEN* pThis, UNLOCKARG lockRect)
	{
		HRESULT rv;

		DDSCAPSN caps;
		GetCaps(pThis, &caps);
		BOOL isPrimary = (caps.dwCaps & (DDSCAPS_PRIMARYSURFACE|DDSCAPS_FRONTBUFFER));

		ddrawdebugprintf(__FUNCTION__ "(0x%X, primary=%d) called.\n", pThis, isPrimary);

		if(!isPrimary || IDirectDrawSurfaceTraits<IDirectDrawSurfaceN>::NUMBER > 3) // hack
		{
			rv = Unlock(pThis, lockRect);
		}
		else
		{
			// some games (like Elastomania and Garden of Coloured Lights)
			// lock the frontbuffer and blit to it themselves, instead of calling Blt.
			// so, we try to convert that into a call to Blt from the backbuffer to the frontbuffer,
			// so that we can get proper window clipping and AVI output. (but maybe not yet there)
			DIRECTDRAWSURFACEN* pBackbuffer;
			GetAttachedFakeBackBuf(pThis, &pBackbuffer);
			if(!pBackbuffer)
			{
				rv = Unlock(pThis, lockRect);
				if(VerifyIsTrustedCaller(!tls.callerisuntrusted)) // needed for rescue: the beagles (must avoid setting usingSDLOrDD there, at least the way MySwapBuffers was originally) and could help avoid spurious extra frames in some other games
					HandleNewFrame(0, 0, pThis, pThis, (LPRECT)lockRect, (LPRECT)lockRect);
			}
			else
			{
				rv = Unlock(pBackbuffer, lockRect);
				s_theBackBuffer = pBackbuffer;

#if 0			// first approach, but covers the screen with crap on startup of tumiki fighters
				MyBltFast(pThis, 0,0,pBackbuffer,(LPRECT)NULL,DDBLTFAST_WAIT);
#else
				// better? or maybe this is usually doing nothing
				MyBlt(pThis, (LPRECT)lockRect,pBackbuffer,(LPRECT)lockRect,0,NULL);
#endif
				//cmdprintf("DEBUGPAUSE: 4");
				if(VerifyIsTrustedCaller(!tls.callerisuntrusted)) // needed for rescue: the beagles (must avoid setting usingSDLOrDD there, at least the way MySwapBuffers was originally) and could help avoid spurious extra frames in some other games
					HandleNewFrame(0, 0, pThis, pBackbuffer, (LPRECT)lockRect, (LPRECT)lockRect);
			}
		}

		videoMemoryBackupDirty[pThis] = TRUE;

		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *GetDC)(DIRECTDRAWSURFACEN* pThis, HDC* lphDC);
	static HRESULT STDMETHODCALLTYPE MyGetDC(DIRECTDRAWSURFACEN* pThis, HDC* lphDC)
	{
		if(!lphDC)
			return DDERR_INVALIDPARAMS;

		// normal case
		HRESULT rv = GetDC(pThis, lphDC);
		ddrawdebugprintf(__FUNCTION__ " called. returned 0x%X, got 0x%X\n", rv, lphDC?*lphDC:0);
		
		if(FAILED(rv))
		{
			// fallback for video cards that have bugs with GetDC on a DirectDraw surface
			DDSURFACEDESCN desc = { sizeof(DDSURFACEDESCN), DDSD_HEIGHT };
			if(FAILED(GetSurfaceDesc(pThis,&desc)))
				{	*lphDC = NULL; return DDERR_CANTCREATEDC; }
			HDC hdcWnd = ::GetDC(gamehwnd);
			HDC hdc = ::CreateCompatibleDC(hdcWnd);
			if(!hdc || FAILED(Lock(pThis,NULL,&desc,DDLOCK_WAIT|DDLOCK_READONLY,NULL)))
				{	*lphDC = NULL; ::ReleaseDC(gamehwnd,hdcWnd); return DDERR_CANTCREATEDC; }
			int width = desc.lPitch * 8 / desc.ddpfPixelFormat.dwRGBBitCount;
			int height = desc.dwHeight;
			HBITMAP hbmp = ::CreateCompatibleBitmap(hdcWnd,width,height);
			::ReleaseDC(gamehwnd,hdcWnd);
			struct { BITMAPINFO bmi;
				RGBQUAD remainingColors [255];
			} fbmi = {sizeof(BITMAPINFOHEADER)};
			BITMAPINFO& bmi = *(BITMAPINFO*)&fbmi;
			::GetDIBits(hdc, hbmp, 0, 0, 0, &bmi, DIB_RGB_COLORS);
			bmi.bmiHeader.biHeight = -height;
			bmi.bmiHeader.biCompression = BI_RGB;
			::SetDIBits(hdc,hbmp,0,height,desc.lpSurface,&bmi,0);
			Unlock(pThis,NULL);
			ManualHDCInfo info;
			info.oldBitmap = (HBITMAP)::SelectObject(hdc, hbmp);
			manuallyCreatedSurfaceDCs[hdc] = info;
			*lphDC = hdc;
		}
		return DD_OK;
	}

	static HRESULT(STDMETHODCALLTYPE *ReleaseDC)(DIRECTDRAWSURFACEN* pThis, HDC hdc);
	static HRESULT STDMETHODCALLTYPE MyReleaseDC(DIRECTDRAWSURFACEN* pThis, HDC hdc)
	{
		ddrawdebugprintf(__FUNCTION__ " called.\n");
		std::map<HDC,ManualHDCInfo>::iterator found = manuallyCreatedSurfaceDCs.find(hdc);
		if(found == manuallyCreatedSurfaceDCs.end())
		{
			// normal case
			HRESULT rv = ReleaseDC(pThis, hdc);
			if(SUCCEEDED(rv))
				videoMemoryBackupDirty[pThis] = TRUE;
			return rv;
		}

		// fallback for video cards that have bugs with GetDC on a DirectDraw surface
		ManualHDCInfo info = found->second;
		manuallyCreatedSurfaceDCs.erase(found);
		DDSURFACEDESCN desc = { sizeof(DDSURFACEDESCN), DDSD_HEIGHT };
		GetSurfaceDesc(pThis, &desc);
		Lock(pThis,NULL,&desc,DDLOCK_WAIT|DDLOCK_WRITEONLY,NULL);
		int width = desc.lPitch * 8 / desc.ddpfPixelFormat.dwRGBBitCount;
		int height = desc.dwHeight;
		HBITMAP hbmp = (HBITMAP)SelectObject(hdc, info.oldBitmap);
		struct { BITMAPINFO bmi;
			RGBQUAD remainingColors [255];
		} fbmi = {sizeof(BITMAPINFOHEADER)};
		BITMAPINFO& bmi = *(BITMAPINFO*)&fbmi;
		::GetDIBits(hdc, hbmp, 0, 0, 0, &bmi, DIB_RGB_COLORS);
		bmi.bmiHeader.biHeight = -height;
		bmi.bmiHeader.biCompression = BI_RGB;
		::GetDIBits(hdc,hbmp,0,height,desc.lpSurface,&bmi,0);
		Unlock(pThis,NULL);
		::DeleteObject((HGDIOBJ)hbmp);
		::DeleteDC(hdc);
		return DD_OK;
	}

	static HRESULT(STDMETHODCALLTYPE *GetFlipStatus)(DIRECTDRAWSURFACEN* pThis, DWORD flags);
	static HRESULT STDMETHODCALLTYPE MyGetFlipStatus(DIRECTDRAWSURFACEN* pThis, DWORD flags)
	{
		HRESULT rv = GetFlipStatus(pThis, flags);
		debugprintf(__FUNCTION__ " called, return 0x%X\n", rv);
		return rv;
	}

    static HRESULT(STDMETHODCALLTYPE *SetPalette)(DIRECTDRAWSURFACEN* pThis, LPDIRECTDRAWPALETTE pPalette);
    static HRESULT STDMETHODCALLTYPE MySetPalette(DIRECTDRAWSURFACEN* pThis, LPDIRECTDRAWPALETTE pPalette)
	{
		debugprintf(__FUNCTION__ " called.\n");
		HRESULT rv = SetPalette(pThis, pPalette);
		if(tasflags.forceWindowed)
		{
			if(FAILED(rv))
			{
				rv = DD_OK;
			}
		}
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *SetColorKey)(DIRECTDRAWSURFACEN* pThis, DWORD dwFlags, LPDDCOLORKEY pKey);
    static HRESULT STDMETHODCALLTYPE MySetColorKey(DIRECTDRAWSURFACEN* pThis, DWORD dwFlags, LPDDCOLORKEY pKey)
	{
		HRESULT rv = SetColorKey(pThis, dwFlags, pKey);
		if(pKey)
			ddrawdebugprintf(__FUNCTION__ "(0x%X, 0x%X - 0x%X) called, hr=0x%X.\n", dwFlags, pKey->dwColorSpaceLowValue, pKey->dwColorSpaceHighValue, rv);
		else
			ddrawdebugprintf(__FUNCTION__ "(0x%X, NULL) called, hr=0x%X.\n", dwFlags, rv);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *GetAttachedSurface)(DIRECTDRAWSURFACEN* pThis, DDSCAPSN* lpddscaps, DIRECTDRAWSURFACEN** ppsurf);
	static HRESULT STDMETHODCALLTYPE MyGetAttachedSurface(DIRECTDRAWSURFACEN* pThis, DDSCAPSN* lpddscaps, DIRECTDRAWSURFACEN** ppsurf)
	{
		if(!lpddscaps || !ppsurf)
			return DDERR_INVALIDPARAMS;

		if(lpddscaps->dwCaps & DDSCAPS_BACKBUFFER)
		{
			GetAttachedFakeBackBuf(pThis, ppsurf);
			if(*ppsurf)
			{
				HRESULT rv = DD_OK;
				ddrawdebugprintf(__FUNCTION__ "(dwCaps=0x%X) called. returned 0x%X, got 0x%X\n", lpddscaps->dwCaps, rv, *ppsurf);
				return rv;
			}
		}

		HRESULT rv = GetAttachedSurface(pThis, lpddscaps, ppsurf);
		ddrawdebugprintf(__FUNCTION__ "(dwCaps=0x%X) called. returned 0x%X, got 0x%X\n", lpddscaps->dwCaps, rv, *ppsurf);
		return rv;
	}


	// some functions I don't really want to hook, except to check if they're getting called (for debugging).
	// although if they are getting called then some of them might need to be hooked to better support fake fullscreen mode.

	//static HRESULT(STDMETHODCALLTYPE *AddAttachedSurface)(DIRECTDRAWSURFACEN* pThis, DIRECTDRAWSURFACEN* lpsurf);
	//static HRESULT STDMETHODCALLTYPE MyAddAttachedSurface(DIRECTDRAWSURFACEN* pThis, DIRECTDRAWSURFACEN* lpsurf)
	//{
	//	HRESULT rv = AddAttachedSurface(pThis, lpsurf);
	//	ddrawdebugprintf(__FUNCTION__ " called. returned 0x%X\n", rv);
	//	return rv;
	//}

 //   static HRESULT(STDMETHODCALLTYPE *GetBltStatus)(DIRECTDRAWSURFACEN* pThis, DWORD dwFlags);
 //   static HRESULT STDMETHODCALLTYPE MyGetBltStatus(DIRECTDRAWSURFACEN* pThis, DWORD dwFlags)
	//{
	//	HRESULT rv = GetBltStatus(pThis, dwFlags);
	//	ddrawdebugprintf(__FUNCTION__ " called. returned 0x%X\n", rv);
	//	return rv;
	//}

	//static HRESULT(STDMETHODCALLTYPE *DeleteAttachedSurface)(DIRECTDRAWSURFACEN* pThis, DWORD a, LPDIRECTDRAWSURFACE b);
	//static HRESULT STDMETHODCALLTYPE MyDeleteAttachedSurface(DIRECTDRAWSURFACEN* pThis, DWORD a, LPDIRECTDRAWSURFACE b)
	//{
	//	HRESULT rv = DeleteAttachedSurface(pThis, a, b);
	//	ddrawdebugprintf(__FUNCTION__ " called. returned 0x%X\n", rv);
	//	return rv;
	//}

	//static HRESULT(STDMETHODCALLTYPE *EnumAttachedSurfaces)(DIRECTDRAWSURFACEN* pThis, LPVOID a, LPDDENUMSURFACESCALLBACK b);
	//static HRESULT STDMETHODCALLTYPE MyEnumAttachedSurfaces(DIRECTDRAWSURFACEN* pThis, LPVOID a, LPDDENUMSURFACESCALLBACK b)
	//{
	//	HRESULT rv = EnumAttachedSurfaces(pThis, a, b);
	//	ddrawdebugprintf(__FUNCTION__ " called. returned 0x%X\n", rv);
	//	return rv;
	//}

	//static HRESULT(STDMETHODCALLTYPE *EnumOverlayZOrders)(DIRECTDRAWSURFACEN* pThis, DWORD a, LPVOID b, LPDDENUMSURFACESCALLBACK c);
	//static HRESULT STDMETHODCALLTYPE MyEnumOverlayZOrders(DIRECTDRAWSURFACEN* pThis, DWORD a, LPVOID b, LPDDENUMSURFACESCALLBACK c)
	//{
	//	HRESULT rv = EnumOverlayZOrders(pThis, a, b, c);
	//	ddrawdebugprintf(__FUNCTION__ " called. returned 0x%X\n", rv);
	//	return rv;
	//}

	//static HRESULT(STDMETHODCALLTYPE *GetClipper)(DIRECTDRAWSURFACEN* pThis, LPDIRECTDRAWCLIPPER FAR* ppClip);
	//static HRESULT STDMETHODCALLTYPE MyGetClipper(DIRECTDRAWSURFACEN* pThis, LPDIRECTDRAWCLIPPER FAR* ppClip)
	//{
	//	HRESULT rv = GetClipper(pThis, ppClip);
	//	ddrawdebugprintf(__FUNCTION__ " called. returned 0x%X, got 0x%X\n", rv, ppClip?*ppClip:0);
	//	return rv;
	//}

	//static HRESULT(STDMETHODCALLTYPE *IsLost)(DIRECTDRAWSURFACEN* pThis);
	//static HRESULT STDMETHODCALLTYPE MyIsLost(DIRECTDRAWSURFACEN* pThis)
	//{
	//	HRESULT rv = IsLost(pThis);
	//	ddrawdebugprintf(__FUNCTION__ " called. returned 0x%X\n", rv);
	//	return rv;
	//}





	static void GetAttachedFakeBackBuf(DIRECTDRAWSURFACEN* pThis, DIRECTDRAWSURFACEN** ppsurf)
	{
		if(ppsurf)
		{
			*ppsurf = attachedSurfaces[pThis];
			ddrawdebugprintf(__FUNCTION__ " got: 0x%X -> 0x%X\n", pThis, *ppsurf);
		}
	}

	static void SetAttachedFakeBackBuf(DIRECTDRAWSURFACEN* pThis, DIRECTDRAWSURFACEN* lpsurf)
	{
		ddrawdebugprintf(__FUNCTION__ " set: 0x%X -> 0x%X\n", pThis, lpsurf);
		attachedSurfaces[pThis] = lpsurf;
	}

	static void BackupVideoMemory(DIRECTDRAWSURFACEN* pThis)
	{
		if(!videoMemoryBackupDirty[pThis])
			return;
		DDSURFACEDESCN desc = { sizeof(DDSURFACEDESCN) };
		if(SUCCEEDED(GetCaps(pThis, &desc.ddsCaps)))
		{
			if(desc.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY
			&& !(desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_FRONTBUFFER)))
			{
				void*& pixels = videoMemoryPixelBackup[pThis];
				ddrawdebugprintf("videoMemoryPixelBackup[0x%X] was 0x%X\n", pThis, pixels);
				if(SUCCEEDED(Lock(pThis, NULL, &desc, DDLOCK_WAIT|DDLOCK_READONLY|DDLOCK_NOSYSLOCK, NULL)))
				{
					int size = desc.lPitch * desc.dwHeight;
					pixels = realloc(pixels, size);
					memcpy(pixels, desc.lpSurface, size);
					Unlock(pThis, NULL);
					ddrawdebugprintf("videoMemoryPixelBackup[0x%X] is 0x%X, size=0x%X\n", pThis, pixels, size);
					// ddrawdebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
				}
			}
		}
		videoMemoryBackupDirty[pThis] = FALSE;
	}

	static void RestoreVideoMemory(DIRECTDRAWSURFACEN* pThis)
	{
		DDSURFACEDESCN desc = { sizeof(DDSURFACEDESCN) };
		if(SUCCEEDED(GetCaps(pThis, &desc.ddsCaps)))
		{
			if(desc.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY
			&& !(desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_FRONTBUFFER)))
			{
				void*& pixels = videoMemoryPixelBackup[pThis];
				if(pixels)
				{
					if(!videoMemoryBackupDirty[pThis])
					{
						ddrawdebugprintf("videoMemoryPixelBackup[0x%X] was 0x%X (restoring)\n", pThis, pixels);
						if(SUCCEEDED(Lock(pThis, NULL, &desc, DDLOCK_WAIT|DDLOCK_WRITEONLY|DDLOCK_NOSYSLOCK, NULL)))
						{
							int size = desc.lPitch * desc.dwHeight;
							memcpy(desc.lpSurface, pixels, size);
							Unlock(pThis, NULL);
							ddrawdebugprintf("videoMemoryPixelBackup[0x%X] is 0x%X, size=0x%X (restoring)\n", pThis, pixels, size);
							// ddrawdebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
						}
					}
					else
					{
						ddrawdebugprintf("videoMemoryPixelBackup[0x%X] was 0x%X (skipped restoring)\n", pThis, pixels);
					}
				}
			}
		}
	}

	static void RestoreVideoMemoryOfAllSurfaces()
	{
		std::map<IDirectDrawSurfaceN*, void*>::iterator iter;
		for(iter = videoMemoryPixelBackup.begin(); iter != videoMemoryPixelBackup.end(); iter++)
			if(iter->second)
				RestoreVideoMemory(iter->first);
	}

	static void BackupVideoMemoryOfAllSurfaces()
	{
		std::map<IDirectDrawSurfaceN*, BOOL>::iterator iter;
		for(iter = videoMemoryBackupDirty.begin(); iter != videoMemoryBackupDirty.end(); iter++)
			if(iter->second)
				BackupVideoMemory(iter->first);
	}

	static void CreateSysMemSurfaceB(DIRECTDRAWSURFACEN*& pDest, DIRECTDRAWSURFACEN* pSource, IDirectDrawOwnerInfo& ownerInfo, DDSURFACEDESCN& ddsd)
	{
		switch(ownerInfo.ddrawVersionNumber)
		{
		case 1:{
			IDirectDraw* dd = (IDirectDraw*)ownerInfo.ddrawInterface;
			dd->CreateSurface((LPDDSURFACEDESC)&ddsd, (LPDIRECTDRAWSURFACE*)&pDest, NULL);
		}	break;
		case 2:{
			IDirectDraw2* dd = (IDirectDraw2*)ownerInfo.ddrawInterface;
			dd->CreateSurface((LPDDSURFACEDESC)&ddsd, (LPDIRECTDRAWSURFACE*)&pDest, NULL);
		}	break;
		case 4:{
			IDirectDraw4* dd = (IDirectDraw4*)ownerInfo.ddrawInterface;
			dd->CreateSurface((LPDDSURFACEDESC2)&ddsd, (LPDIRECTDRAWSURFACE4*)&pDest, NULL);
		}	break;
		case 7:{
			IDirectDraw7* dd = (IDirectDraw7*)ownerInfo.ddrawInterface;
			dd->CreateSurface((LPDDSURFACEDESC2)&ddsd, (LPDIRECTDRAWSURFACE7*)&pDest, NULL);
		}	break;
		}
	}

	static DIRECTDRAWSURFACEN* CreateSysMemSurface(DIRECTDRAWSURFACEN* pSource, IDirectDrawOwnerInfo ownerInfo)
	{
		if(!ownerInfo.ddrawInterface)
			return NULL;
		DDSURFACEDESCN ddsdesc = { sizeof(DDSURFACEDESCN), DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT };
		if(FAILED(GetSurfaceDesc(pSource, &ddsdesc)))
			return NULL;
		if(ddsdesc.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
			return pSource; // if it's already in system memory, there's no need to create a new surface
		ddsdesc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
		DIRECTDRAWSURFACEN* pSysMemSurface = NULL;
		CreateSysMemSurfaceB(pSysMemSurface, pSource, ownerInfo, ddsdesc);
		ddrawdebugprintf("created avi system memory surface (%dx%d) = 0x%X\n", ddsdesc.dwWidth, ddsdesc.dwHeight, pSysMemSurface);
		return pSysMemSurface;
	}

	static std::map<IDirectDrawSurfaceN*, IDirectDrawSurfaceN*> attachedSurfaces;
	static std::map<IDirectDrawSurfaceN*, void*> videoMemoryPixelBackup;
	static std::map<IDirectDrawSurfaceN*, BOOL> videoMemoryBackupDirty;
	static std::map<IDirectDrawSurfaceN*, IDirectDrawSurfaceN*> sysMemCopySurfaces; // for avi speedup
};


// stupid C++, have to define statics outside the class,
// and different compiler versions have different bugs with it.
// this (macro-based) way of defining them was the only one that worked on all the compilers I tried

#define HRESULT long // some versions of visual studio (including 2008) don't understand "typedef long HRESULT;" (the compiler claims HRESULT is an undefined type even after it's typedef'd to long)

#define DEF(x) template<> ULONG (STDMETHODCALLTYPE* MyDirectDrawSurface<x>::Release)(x* pThis) = 0; \
               template<> HRESULT(STDMETHODCALLTYPE* MyDirectDrawSurface<x>::QueryInterface)(x* pThis, REFIID riid, void** ppvObj) = 0; \
               template<> HRESULT(STDMETHODCALLTYPE* MyDirectDrawSurface<x>::Blt)(x* pThis, LPRECT a,DIRECTDRAWSURFACEN* b, LPRECT c,DWORD d, LPDDBLTFX e) = 0; \
               template<> HRESULT(STDMETHODCALLTYPE* MyDirectDrawSurface<x>::BltFast)(x* pThis, DWORD a,DWORD b,DIRECTDRAWSURFACEN* c, LPRECT d,DWORD e) = 0; \
               template<> HRESULT(STDMETHODCALLTYPE* MyDirectDrawSurface<x>::BltBatch)(x* pThis, LPDDBLTBATCH a, DWORD b, DWORD c) = 0; \
               template<> HRESULT(STDMETHODCALLTYPE* MyDirectDrawSurface<x>::GetCaps)(x* pThis, DDSCAPSN* lpddscaps) = 0; \
               template<> HRESULT(STDMETHODCALLTYPE* MyDirectDrawSurface<x>::GetSurfaceDesc)(x* pThis, DDSURFACEDESCN* lpddsdesc) = 0; \
               template<> HRESULT(STDMETHODCALLTYPE* MyDirectDrawSurface<x>::GetPixelFormat)(x* pThis, LPDDPIXELFORMAT lpddpfmt) = 0; \
               template<> HRESULT(STDMETHODCALLTYPE* MyDirectDrawSurface<x>::Flip)(x* pThis, x* pOther, DWORD flags) = 0; \
               template<> HRESULT(STDMETHODCALLTYPE* MyDirectDrawSurface<x>::Lock)(x* pThis, LPRECT a,DDSURFACEDESCN* b,DWORD c,HANDLE d) = 0; \
               template<> HRESULT(STDMETHODCALLTYPE* MyDirectDrawSurface<x>::Unlock)(x* pThis, UNLOCKARG a) = 0; \
               template<> HRESULT(STDMETHODCALLTYPE* MyDirectDrawSurface<x>::GetDC)(x* pThis, HDC* lphDC) = 0; \
               template<> HRESULT(STDMETHODCALLTYPE* MyDirectDrawSurface<x>::ReleaseDC)(x* pThis, HDC hDC) = 0; \
               template<> HRESULT(STDMETHODCALLTYPE* MyDirectDrawSurface<x>::GetAttachedSurface)(x* pThis, DDSCAPSN* lpddscaps, x** ppsurf) = 0; \
               /*template<> HRESULT(STDMETHODCALLTYPE* MyDirectDrawSurface<x>::AddAttachedSurface)(x* pThis, x* lpsurf) = 0;*/ \
               /*template<> HRESULT(STDMETHODCALLTYPE* MyDirectDrawSurface<x>::GetBltStatus)(x* pThis, DWORD) = 0;*/ \
               /*template<> HRESULT(STDMETHODCALLTYPE* MyDirectDrawSurface<x>::DeleteAttachedSurface)(x* pThis, DWORD,LPDIRECTDRAWSURFACE) = 0;*/ \
               /*template<> HRESULT(STDMETHODCALLTYPE* MyDirectDrawSurface<x>::EnumAttachedSurfaces)(x* pThis, LPVOID,LPDDENUMSURFACESCALLBACK) = 0;*/ \
               /*template<> HRESULT(STDMETHODCALLTYPE* MyDirectDrawSurface<x>::EnumOverlayZOrders)(x* pThis, DWORD,LPVOID,LPDDENUMSURFACESCALLBACK) = 0;*/ \
               /*template<> HRESULT(STDMETHODCALLTYPE* MyDirectDrawSurface<x>::GetClipper)(x* pThis, LPDIRECTDRAWCLIPPER FAR* ppClip) = 0;*/ \
               /*template<> HRESULT(STDMETHODCALLTYPE* MyDirectDrawSurface<x>::IsLost)(x* pThis) = 0;*/ \
               template<> HRESULT(STDMETHODCALLTYPE* MyDirectDrawSurface<x>::GetFlipStatus)(x* pThis, DWORD flags) = 0; \
			   template<> HRESULT(STDMETHODCALLTYPE* MyDirectDrawSurface<x>::SetPalette)(x* pThis, LPDIRECTDRAWPALETTE pPalette) = 0; \
			   template<> HRESULT(STDMETHODCALLTYPE* MyDirectDrawSurface<x>::SetColorKey)(x* pThis, DWORD dwFlags, LPDDCOLORKEY pKey) = 0; \
               template<> std::map<x*,x*> MyDirectDrawSurface<x>::attachedSurfaces; \
               template<> std::map<x*,x*> MyDirectDrawSurface<x>::sysMemCopySurfaces; \
               template<> std::map<x*,void*> MyDirectDrawSurface<x>::videoMemoryPixelBackup; \
               template<> std::map<x*,BOOL> MyDirectDrawSurface<x>::videoMemoryBackupDirty;

	DEF(IDirectDrawSurface)
	DEF(IDirectDrawSurface2)
	DEF(IDirectDrawSurface3)
	DEF(IDirectDrawSurface4)
	DEF(IDirectDrawSurface7)
#undef DEF

#undef HRESULT


void BackupVideoMemoryOfAllDDrawSurfaces()
{
	MyDirectDrawSurface<IDirectDrawSurface>::BackupVideoMemoryOfAllSurfaces();
	MyDirectDrawSurface<IDirectDrawSurface2>::BackupVideoMemoryOfAllSurfaces();
	MyDirectDrawSurface<IDirectDrawSurface3>::BackupVideoMemoryOfAllSurfaces();
	MyDirectDrawSurface<IDirectDrawSurface4>::BackupVideoMemoryOfAllSurfaces();
	MyDirectDrawSurface<IDirectDrawSurface7>::BackupVideoMemoryOfAllSurfaces();
}

void RestoreVideoMemoryOfAllDDrawSurfaces()
{
	MyDirectDrawSurface<IDirectDrawSurface>::RestoreVideoMemoryOfAllSurfaces();
	MyDirectDrawSurface<IDirectDrawSurface2>::RestoreVideoMemoryOfAllSurfaces();
	MyDirectDrawSurface<IDirectDrawSurface3>::RestoreVideoMemoryOfAllSurfaces();
	MyDirectDrawSurface<IDirectDrawSurface4>::RestoreVideoMemoryOfAllSurfaces();
	MyDirectDrawSurface<IDirectDrawSurface7>::RestoreVideoMemoryOfAllSurfaces();
}

bool RedrawScreenDDraw()
{
	if(dds_bltsaved)
	{
		//debugprintf("(%X,%X,%X,%X,%X).\n", a_bltsaved,b_bltsaved,c_bltsaved,d_bltsaved,e_bltsaved);
		HRESULT rv = dds_bltsaved->Blt(a_bltsaved,b_bltsaved,c_bltsaved,d_bltsaved,e_bltsaved);
		return true;
	}
	return false;
}








template <typename IDirectDrawN>
class MyDirectDraw : public IDirectDrawN
{
public:
	typedef typename IDirectDrawTraits<IDirectDrawN>::DIRECTDRAWSURFACEN DIRECTDRAWSURFACEN;
	typedef typename IDirectDrawTraits<IDirectDrawN>::DDSURFACEDESCN DDSURFACEDESCN;
	typedef typename IDirectDrawTraits<IDirectDrawN>::DDSCAPSN DDSCAPSN;
	typedef typename IDirectDrawTraits<IDirectDrawN>::DDDEVICEIDENTIFIERN DDDEVICEIDENTIFIERN;
	typedef HRESULT (FAR PASCAL * LPDDENUMMODESCALLBACKN)(DDSURFACEDESCN*, LPVOID);
	typedef HRESULT (FAR PASCAL * LPDDENUMSURFACESCALLBACKN)(DIRECTDRAWSURFACEN*, DDSURFACEDESCN*, LPVOID);
	static const GUID IID_IDirectDrawSurfaceN;

	MyDirectDraw(IDirectDrawN* dd) : m_dd(dd)
	{
//		debugprintf(__FUNCTION__ ": m_dd = 0x%X\n", m_dd);
		ddrawdebugprintf(__FUNCTION__ " called (#%d).\n", ++s_numMyDirectDraws);
	}
	~MyDirectDraw()
	{
		ddrawdebugprintf(__FUNCTION__ " called (#%d).\n", --s_numMyDirectDraws);
	}

	/*** IUnknown methods ***/
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj)
	{
		ddrawdebugprintf(__FUNCTION__ "(0x%X) called.\n", riid.Data1);
		HRESULT rv = m_dd->QueryInterface(riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

    ULONG STDMETHODCALLTYPE AddRef()
	{
		ddrawdebugprintf(__FUNCTION__ " called.\n");
		return m_dd->AddRef();
	}

    ULONG STDMETHODCALLTYPE Release()
	{
		ddrawdebugprintf(__FUNCTION__ " called.\n");
		ULONG count = m_dd->Release();
		if(0 == count)
			delete this;

		return count;
	}

    /*** IDirectDraw methods ***/
    STDMETHOD(Compact)()
	{
		ddrawdebugprintf(__FUNCTION__ " called.\n");
		return m_dd->Compact();
	}
    STDMETHOD(CreateClipper)(DWORD a, LPDIRECTDRAWCLIPPER FAR* b, IUnknown FAR * c)
	{
		ddrawdebugprintf(__FUNCTION__ " called.\n");
		return m_dd->CreateClipper(a,b,c);
	}
    STDMETHOD(CreatePalette)(DWORD dwFlags, LPPALETTEENTRY b, LPDIRECTDRAWPALETTE FAR* c, IUnknown FAR * d)
	{
		if(tasflags.forceWindowed)
		{
			dwFlags &= ~(DDPCAPS_PRIMARYSURFACE|DDPCAPS_PRIMARYSURFACELEFT|DDPCAPS_VSYNC);
		}
		HRESULT rv = m_dd->CreatePalette(dwFlags,b,c,d);
		ddrawdebugprintf(__FUNCTION__ " called, hr=0x%X.\n", rv);
		return rv;
	}
    STDMETHOD(CreateSurface)( DDSURFACEDESCN* lpddsd, DIRECTDRAWSURFACEN* FAR * ppdds, IUnknown FAR * pUnkOuter)
	{
		if(!lpddsd)
			return E_POINTER;
		//DDSURFACEDESCN& ddsd = *lpddsd;
		DDSURFACEDESCN ddsd;
		memcpy(&ddsd, lpddsd, sizeof(DDSURFACEDESCN));
		bool isPrimary = (ddsd.dwFlags & DDSD_CAPS) && (ddsd.ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE);
		bool autoAttachSurface = false;
		bool autoClipSurface = false;
		if(isPrimary && (!(ddsd.dwFlags & DDSD_BACKBUFFERCOUNT) || !ddsd.dwBackBufferCount) && IDirectDrawTraits<IDirectDrawN>::NUMBER <= 2) // hack: version check is to avoid breaking BMD.
		{
			// for elastomania (maybe) and garden of coloured lights
			// ... for games that normally only use the front buffer,
			// we want to force having a back buffer for our own use
			// (to help avi capture and maybe depth conversion)
			autoAttachSurface = true;
		}
		if(tasflags.forceWindowed)
		{
			//ddrawdebugprintf("ddsd.ddsCaps.dwCaps=0x%X\n", ddsd.ddsCaps.dwCaps);
			if(isPrimary)
			{
				if((ddsd.ddsCaps.dwCaps & (DDSCAPS_FLIP | DDSCAPS_COMPLEX)) || IsWindowFakeFullscreen(gamehwnd))
					autoClipSurface = true;
				ddsd.ddsCaps.dwCaps &= ~(DDSCAPS_VIDEOMEMORY | DDSCAPS_FLIP | DDSCAPS_COMPLEX);
			}
			if(ddsd.dwFlags & DDSD_BACKBUFFERCOUNT)
			{
				if(ddsd.dwBackBufferCount > 0)
				{
					ddsd.dwFlags &= ~DDSD_BACKBUFFERCOUNT;
					ddsd.dwBackBufferCount = 0;
					autoAttachSurface = true;
				}
			}
			//else if(isPrimary) // I forget what needed this, but it kills BMD. it might be ok if we also check autoClipSurface.
			//{
			//	ddsd.dwBackBufferCount = 0;
			//	autoAttachSurface = true;
			//}
		}

		ddrawdebugprintf(__FUNCTION__ " called.\n");
		ddrawdebugprintf("dwFlags = 0x%X.\n", ddsd.dwFlags);
		ddrawdebugprintf("dwWidth,dwHeight = %d,%d.\n", ddsd.dwWidth,ddsd.dwHeight);
		ddrawdebugprintf("lPitch = %d.\n", ddsd.lPitch);
		ddrawdebugprintf("dwBackBufferCount = %d.\n", ddsd.dwBackBufferCount);
		ddrawdebugprintf("dwRefreshRate = %d.\n", ddsd.dwRefreshRate);
		ddrawdebugprintf("lpSurface = %d.\n", ddsd.lpSurface);
		ddrawdebugprintf("ddckCKSrcOverlay = %d.\n", ddsd.ddckCKSrcOverlay);
		ddrawdebugprintf("ddckCKSrcBlt = %d.\n", ddsd.ddckCKSrcBlt);
		ddrawdebugprintf("ddpfPixelFormat = %d.\n", ddsd.ddpfPixelFormat);
		ddrawdebugprintf("ddsCaps = 0x%X.\n", ddsd.ddsCaps.dwCaps);

		ddsd.ddsCaps.dwCaps &= ~DDSCAPS_WRITEONLY;

		DWORD capsToTry [8];
		int numCapsToTry = 0;

		switch(tasflags.forceSurfaceMemory)
		{
		case 1:
			// forced system memory
			capsToTry[numCapsToTry++] = (ddsd.ddsCaps.dwCaps | DDSCAPS_SYSTEMMEMORY) & ~(DDSCAPS_VIDEOMEMORY|DDSCAPS_LOCALVIDMEM|DDSCAPS_NONLOCALVIDMEM);
			break;
		case 2:
			// non-local video memory
			capsToTry[numCapsToTry++] = (ddsd.ddsCaps.dwCaps | DDSCAPS_VIDEOMEMORY | DDSCAPS_NONLOCALVIDMEM) & ~(DDSCAPS_SYSTEMMEMORY|DDSCAPS_LOCALVIDMEM);
			break;
		case 3:
			// local video memory
			capsToTry[numCapsToTry++] = (ddsd.ddsCaps.dwCaps | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM) & ~(DDSCAPS_SYSTEMMEMORY|DDSCAPS_NONLOCALVIDMEM);
			break;
		}

		// game's choice
		capsToTry[numCapsToTry++] = ddsd.ddsCaps.dwCaps;
		// anything
		capsToTry[numCapsToTry++] = ddsd.ddsCaps.dwCaps & ~(DDSCAPS_SYSTEMMEMORY|DDSCAPS_VIDEOMEMORY|DDSCAPS_LOCALVIDMEM|DDSCAPS_NONLOCALVIDMEM);


		HRESULT hr = DDERR_GENERIC;
		for(int i = 0; FAILED(hr) && i < ARRAYSIZE(capsToTry); i++)
		{
			ddsd.ddsCaps.dwCaps = capsToTry[i];
			hr = m_dd->CreateSurface(&ddsd, ppdds, pUnkOuter);
		}

		if(SUCCEEDED(hr))
		{
			//debugprintf("A: 0x%X\n", ddsd.ddsCaps.dwCaps);
			//ddsd.ddsCaps.dwCaps = 0;
			//(*ppdds)->GetCaps(&ddsd.ddsCaps);
			//debugprintf("B: 0x%X\n", ddsd.ddsCaps.dwCaps);

			// Return our own directdraw surface so we can spy on it
			//MyDirectDrawSurface<DIRECTDRAWSURFACEN>::Hook(*ppdds);
			HookCOMInterface(IID_IDirectDrawSurfaceN, (LPVOID*)ppdds, true);

			if(autoAttachSurface)
			{
				// this is for faking fullscreen mode by creating and controlling our own backbuffer
				// instead of letting directdraw create the back buffer automatically
				ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
				if(IDirectDrawTraits<IDirectDrawN>::NUMBER == 7)
					ddsd.ddsCaps.dwCaps |= DDSCAPS_3DDEVICE; // fixes various d3d7 games (such as Ninjah and maybe BMD) when in fake fullscreen mode: Direct3DCreateDevice7 will fail if we don't request this capability for our backbuffer
				ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
				if(fakeDisplayValid)
				{
					ddsd.dwWidth = fakeDisplayWidth;
					ddsd.dwHeight = fakeDisplayHeight;
					//if(fakePixelFormatBPP) ddsd.ddpfPixelFormat.dwRGBBitCount = fakePixelFormatBPP;
				}
				else
				{
					DDSURFACEDESCN ddsd2 = { sizeof(DDSURFACEDESCN), DDSD_WIDTH | DDSD_HEIGHT };
					m_dd->GetDisplayMode(&ddsd2);
					ddsd.dwWidth = ddsd2.dwWidth;
					ddsd.dwHeight = ddsd2.dwHeight;
				}

				DIRECTDRAWSURFACEN* pAttachedSurface = NULL;
				debugprintf("creating attached surface (%dx%d)...\n", ddsd.dwWidth, ddsd.dwHeight);
				HRESULT hr = CreateSurface(&ddsd, &pAttachedSurface, NULL);
				if(SUCCEEDED(hr))
				{
					MyDirectDrawSurface<DIRECTDRAWSURFACEN>::SetAttachedFakeBackBuf(*ppdds, pAttachedSurface);
					if(isPrimary)
					{
						// since IDirectDrawSurface doesn't always provide a way to access the IDirectDraw interface,
						// store enough info to get this interface given a pointer to the primary surface.
						IDirectDrawOwnerInfo info = {(void*)this, IDirectDrawTraits<IDirectDrawN>::NUMBER};
						s_ddrawSurfaceToOwner[(LPVOID)pAttachedSurface] = info;
					}
				}
			}

			if(isPrimary)
			{
				// since IDirectDrawSurface doesn't always provide a way to access the IDirectDraw interface,
				// store enough info to get this interface given a pointer to the primary surface.
				IDirectDrawOwnerInfo info = {(void*)this, IDirectDrawTraits<IDirectDrawN>::NUMBER};
				s_ddrawSurfaceToOwner[*(LPVOID*)ppdds] = info;
			}

			if(autoClipSurface && gamehwnd)
			{
				debugprintf("auto setting clipper (surface=0x%X, hwnd=0x%X)...\n", *ppdds, gamehwnd);
				// also set the clipper, since the game won't do that
				// since it thinks it's in fullscreen mode.
				LPDIRECTDRAWCLIPPER pClipper;
				if(SUCCEEDED(m_dd->CreateClipper(0, &pClipper, NULL)))
					if(SUCCEEDED(pClipper->SetHWnd(0, gamehwnd)))
						(*ppdds)->SetClipper(pClipper);
				//if(SUCCEEDED(m_dd->CreateClipper(0, &pClipper, NULL)))
				//	if(SUCCEEDED(pClipper->SetHWnd(0, gamehwnd)))
				//		(pAttachedSurface)->SetClipper(pClipper);
			}
		}
		else
		{
			debugprintf("CreateSurface failed with error 0x%X\n", hr);

		    // ((HRESULT) (((unsigned long)(1)<<31) | ((unsigned long)(0x876)<<16) | ((unsigned long)(code))) )
			// 0x887600E1
			// DDERR_NOEXCLUSIVEMODE
		}

		memcpy(&ddsd, lpddsd, sizeof(DDSURFACEDESCN));
		ddrawdebugprintf(__FUNCTION__ " returned 0x%X.\n", hr);
		ddrawdebugprintf("dwFlags = 0x%X.\n", ddsd.dwFlags);
		ddrawdebugprintf("dwWidth,dwHeight = %d,%d.\n", ddsd.dwWidth,ddsd.dwHeight);
		ddrawdebugprintf("lPitch = %d.\n", ddsd.lPitch);
		ddrawdebugprintf("dwBackBufferCount = %d.\n", ddsd.dwBackBufferCount);
		ddrawdebugprintf("dwRefreshRate = %d.\n", ddsd.dwRefreshRate);
		ddrawdebugprintf("lpSurface = %d.\n", ddsd.lpSurface);
		ddrawdebugprintf("ddckCKSrcOverlay = %d.\n", ddsd.ddckCKSrcOverlay);
		ddrawdebugprintf("ddckCKSrcBlt = %d.\n", ddsd.ddckCKSrcBlt);
		ddrawdebugprintf("ddpfPixelFormat = %d.\n", ddsd.ddpfPixelFormat);
		ddrawdebugprintf("ddsCaps = 0x%X.\n", ddsd.ddsCaps.dwCaps);

		return hr;
	}
    STDMETHOD(DuplicateSurface)( DIRECTDRAWSURFACEN* a, DIRECTDRAWSURFACEN* FAR * b)
	{
		ddrawdebugprintf(__FUNCTION__ " called.\n");
		HRESULT rv = m_dd->DuplicateSurface(a,b);
		if(SUCCEEDED(rv))
			HookCOMInterface(IID_IDirectDrawSurfaceN, (LPVOID*)b);
		return rv;
	}
    STDMETHOD(EnumDisplayModes)( DWORD a, DDSURFACEDESCN* b, LPVOID c, LPDDENUMMODESCALLBACKN d)
	{
		ddrawdebugprintf(__FUNCTION__ " called.\n");
		return m_dd->EnumDisplayModes(a,b,c,d);
	}
    STDMETHOD(EnumSurfaces)(DWORD a, DDSURFACEDESCN* b, LPVOID c,LPDDENUMSURFACESCALLBACKN d)
	{
		ddrawdebugprintf(__FUNCTION__ " called.\n");
		return m_dd->EnumSurfaces(a,b,c,d);
	}
    STDMETHOD(FlipToGDISurface)()
	{
		ddrawdebugprintf(__FUNCTION__ " called.\n");
		return m_dd->FlipToGDISurface();
	}
    STDMETHOD(GetCaps)( LPDDCAPS a, LPDDCAPS b)
	{
		ddrawdebugprintf(__FUNCTION__ " called.\n");
		return m_dd->GetCaps(a,b);
	}
    STDMETHOD(GetDisplayMode)( DDSURFACEDESCN* lpddsd)
	{
		ddrawdebugprintf(__FUNCTION__ " called.\n");
		HRESULT rv = m_dd->GetDisplayMode(lpddsd);
		if(SUCCEEDED(rv) && fakeDisplayValid)
		{
			lpddsd->dwWidth = fakeDisplayWidth;
			lpddsd->dwHeight = fakeDisplayHeight;
			if(fakePixelFormatBPP) lpddsd->ddpfPixelFormat.dwRGBBitCount = fakePixelFormatBPP; // FIXME leaking info to game that might potentially cause desync... but it seems to help avoid graphics issues
		}
		ddrawdebugprintf(__FUNCTION__ " returned (hr=0x%x, %dx%d, %dbpp).\n", rv, lpddsd->dwWidth, lpddsd->dwHeight, lpddsd->ddpfPixelFormat.dwRGBBitCount);
		return rv;
	}
    STDMETHOD(GetFourCCCodes)( LPDWORD a, LPDWORD b)
	{
		ddrawdebugprintf(__FUNCTION__ " called.\n");
		return m_dd->GetFourCCCodes(a,b);
	}
    STDMETHOD(GetGDISurface)(DIRECTDRAWSURFACEN* FAR * a)
	{
		ddrawdebugprintf(__FUNCTION__ " called.\n");
		return m_dd->GetGDISurface(a);
	}
    STDMETHOD(GetMonitorFrequency)(LPDWORD a)
	{
		ddrawdebugprintf(__FUNCTION__ " called.\n");
//		return m_dd->GetMonitorFrequency(a);
		if(fakeDisplayRefresh >= 10 && fakeDisplayRefresh <= 300)
			return fakeDisplayRefresh;
		// TODO: unduplicate, see MyGetDeviceCaps
		int dist50 = abs((tasflags.framerate % 25) - 12);
		int dist60 = abs((tasflags.framerate % 15) - 8) * 3 / 2;
		if(dist60 <= dist50)
			return 60;
		else if(tasflags.framerate < 75)
			return 50;
		else
			return 100;
	}
    STDMETHOD(GetScanLine)(LPDWORD a)
	{
		//if(a)
		//	*a = 200;
		//return DD_OK;
		return DDERR_VERTICALBLANKINPROGRESS; // TESTING
		HRESULT rv = m_dd->GetScanLine(a);
		ddrawdebugprintf(__FUNCTION__ " called. got %d\n", *a);
		return rv;
	}
    STDMETHOD(GetVerticalBlankStatus)(LPBOOL a)
	{
		static int b = 0;
		b = !b;
		if(a)
			*a = b; // TESTING
		return DD_OK; // TESTING
		HRESULT rv = m_dd->GetVerticalBlankStatus(a);
		ddrawdebugprintf(__FUNCTION__ " called. got %d\n", *a);
		return rv;
	}
    STDMETHOD(Initialize)(GUID FAR * a)
	{
		ddrawdebugprintf(__FUNCTION__ " called.\n");
		return m_dd->Initialize(a);
	}
    STDMETHOD(RestoreDisplayMode)()
	{
		ddrawdebugprintf(__FUNCTION__ " called.\n");
		return m_dd->RestoreDisplayMode();
	}
    STDMETHOD(SetCooperativeLevel)(HWND hwnd, DWORD flags)
	{
		if(IsWindow(hwnd))
			gamehwnd = hwnd;
		ddrawdebugprintf(__FUNCTION__ "(0x%X, 0x%X) called.\n", hwnd, flags);

		if(tasflags.forceWindowed)
		{
			flags &= ~DDSCL_FULLSCREEN;
			flags &= ~DDSCL_EXCLUSIVE;
			flags |= DDSCL_NORMAL;
			//flags |= DDSCL_NOWINDOWCHANGES;
		}

		HRESULT rv = m_dd->SetCooperativeLevel(hwnd,flags);
		ddrawdebugprintf(__FUNCTION__ "(0x%X, 0x%X) returned 0x%X.\n", hwnd, flags, rv);

		if(tasflags.forceWindowed && FAILED(rv))
		{
			rv = m_dd->SetCooperativeLevel(hwnd,DDSCL_NORMAL);
			ddrawdebugprintf(__FUNCTION__ "(0x%X, 0x%X) returned 0x%X.\n", hwnd, DDSCL_NORMAL, rv);
		}

		//return rv;
		return DD_OK;
	}
    STDMETHOD(SetDisplayMode)(DWORD width, DWORD height,DWORD bpp) IMPOSSIBLE_IMPL
    STDMETHOD(SetDisplayMode)(DWORD width, DWORD height,DWORD bpp,DWORD refresh,DWORD flags)
	{
		ddrawdebugprintf(__FUNCTION__ "(%dx%d, %dbpp, %dhz) called.\n", width, height, bpp, refresh);
		if(tasflags.forceWindowed)
		{
			if(gamehwnd)
				MakeWindowWindowed(gamehwnd, width, height);
			fakeDisplayWidth = width;
			fakeDisplayHeight = height;
			fakePixelFormatBPP = bpp;
			fakeDisplayRefresh = refresh;
			fakeDisplayValid = TRUE;
			DDSURFACEDESCN ddsd = { sizeof(DDSURFACEDESCN) };
			ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT | DDSD_REFRESHRATE;
			ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
			ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
			m_dd->GetDisplayMode(&ddsd);
			FakeBroadcastDisplayChange(fakeDisplayWidth, fakeDisplayHeight, fakePixelFormatBPP);
			// TODO investigate: la mulana works much better if we just return ok here, and I can't remember what game the rest of this code was needed for (temporal, elastomania)
			//if(bpp == ddsd.ddpfPixelFormat.dwRGBBitCount)
				return DD_OK;
			//width = ddsd.dwWidth;
			//height = ddsd.dwHeight;
			//bpp = bpp;
			//refresh = ddsd.dwRefreshRate;
			//flags = 0;
			//m_dd->SetDisplayMode(width,height,bpp,refresh,flags);
			//return DD_OK;
		}
		HRESULT rv = m_dd->SetDisplayMode(width,height,bpp,refresh,flags);
		FakeBroadcastDisplayChange(width,height,bpp);
		ddrawdebugprintf(__FUNCTION__ "(%dx%d, %dbpp, %dhz) returned 0x%X.\n", width, height, bpp, refresh, rv);
		return rv;
	}
    STDMETHOD(WaitForVerticalBlank)(DWORD a, HANDLE b)
	{
//		if(tasflags.fastForward)
			return DD_OK;
		ddrawdebugprintf(__FUNCTION__ " called.\n");
		return m_dd->WaitForVerticalBlank(a,b);
	}
	
	// IDirectDraw2
	STDMETHOD(GetAvailableVidMem)(DDSCAPSN* a, LPDWORD b, LPDWORD c)
	{
		ddrawdebugprintf(__FUNCTION__ " called.\n");
		return m_dd->GetAvailableVidMem(a,b,c);
	}

	// IDirectDraw4
	STDMETHOD(GetSurfaceFromDC) (HDC a, DIRECTDRAWSURFACEN* * b)
	{
		ddrawdebugprintf(__FUNCTION__ " called.\n");
		return m_dd->GetSurfaceFromDC(a,b);
	}
    STDMETHOD(RestoreAllSurfaces)()
	{
		ddrawdebugprintf(__FUNCTION__ " called.\n");
		return m_dd->RestoreAllSurfaces();
	}
    STDMETHOD(TestCooperativeLevel)()
	{
		ddrawdebugprintf(__FUNCTION__ " called.\n");
		return m_dd->TestCooperativeLevel();
	}
    STDMETHOD(GetDeviceIdentifier)(DDDEVICEIDENTIFIERN* a, DWORD b)
	{
		ddrawdebugprintf(__FUNCTION__ " called.\n");

		ThreadLocalStuff& curtls = tls;

		if(VerifyIsTrustedCaller(!curtls.callerisuntrusted))
		{
			if(!a)
				return DDERR_INVALIDPARAMS;
			memset(a, 0, sizeof(DDDEVICEIDENTIFIERN));
			strcpy(a->szDriver, "hourglass_ddraw");
			strcpy(a->szDescription, "Hourglass DirectDraw Wrapper");
			a->guidDeviceIdentifier.Data1 = 0x12345432;
			return DD_OK;
		}

		const char* oldName = curtls.curThreadCreateName;
		curtls.curThreadCreateName = "DirectDraw";
		curtls.callerisuntrusted++;

		HRESULT rv = m_dd->GetDeviceIdentifier(a,b);

		curtls.callerisuntrusted--;
		curtls.curThreadCreateName = oldName;
		return rv;
	}

	// IDirectDraw7
	STDMETHOD(StartModeTest)(LPSIZE a, DWORD b, DWORD c) IMPOSSIBLE_IMPL
    STDMETHOD(EvaluateMode)(DWORD a, DWORD* b) IMPOSSIBLE_IMPL

private:
	IDirectDrawN* m_dd;
};

template<> HRESULT MyDirectDraw<IDirectDraw>::GetAvailableVidMem(DDSCAPSN* a, LPDWORD b, LPDWORD c) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectDraw<IDirectDraw>::GetSurfaceFromDC(HDC a, DIRECTDRAWSURFACEN* * b) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectDraw<IDirectDraw>::RestoreAllSurfaces() IMPOSSIBLE_IMPL
template<> HRESULT MyDirectDraw<IDirectDraw>::TestCooperativeLevel() IMPOSSIBLE_IMPL
template<> HRESULT MyDirectDraw<IDirectDraw>::GetDeviceIdentifier(LPDDDEVICEIDENTIFIER a, DWORD b) IMPOSSIBLE_IMPL

template<> HRESULT MyDirectDraw<IDirectDraw2>::GetSurfaceFromDC(HDC a, DIRECTDRAWSURFACEN* * b) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectDraw<IDirectDraw2>::RestoreAllSurfaces() IMPOSSIBLE_IMPL
template<> HRESULT MyDirectDraw<IDirectDraw2>::TestCooperativeLevel() IMPOSSIBLE_IMPL
template<> HRESULT MyDirectDraw<IDirectDraw2>::GetDeviceIdentifier(LPDDDEVICEIDENTIFIER a, DWORD b) IMPOSSIBLE_IMPL

template<> HRESULT MyDirectDraw<IDirectDraw>::SetDisplayMode(DWORD width, DWORD height,DWORD bpp,DWORD refresh,DWORD flags) IMPOSSIBLE_IMPL
template<> HRESULT MyDirectDraw<IDirectDraw>::SetDisplayMode(DWORD width, DWORD height,DWORD bpp)
{
	ddrawdebugprintf(__FUNCTION__ "(%dx%d, %dbpp) called.\n", width, height, bpp);
	if(tasflags.forceWindowed)
	{
		if(gamehwnd)
			MakeWindowWindowed(gamehwnd, width, height);
		fakeDisplayWidth = width;
		fakeDisplayHeight = height;
		fakePixelFormatBPP = bpp;
		fakeDisplayValid = TRUE;
		DDSURFACEDESCN ddsd = { sizeof(DDSURFACEDESCN) };
		ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
		ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
		m_dd->GetDisplayMode(&ddsd);
		FakeBroadcastDisplayChange(fakeDisplayWidth, fakeDisplayHeight, fakePixelFormatBPP);
		// TODO investigate: I can't remember what game the rest of this code was needed for
		//if(bpp == ddsd.ddpfPixelFormat.dwRGBBitCount)
			return DD_OK;
		//width = ddsd.dwWidth;
		//height = ddsd.dwHeight;
		//bpp = bpp;
		//m_dd->SetDisplayMode(width,height,bpp);
		//return DD_OK;
	}
	HRESULT rv = m_dd->SetDisplayMode(width,height,bpp);
	FakeBroadcastDisplayChange(width,height,bpp);
	ddrawdebugprintf(__FUNCTION__ "(%dx%d, %dbpp) returned 0x%X.\n", width, height, bpp, rv);
	return rv;
}

template<> HRESULT MyDirectDraw<IDirectDraw7>::StartModeTest(LPSIZE a, DWORD b, DWORD c)
{
	ddrawdebugprintf(__FUNCTION__ " called.\n");
//	if(tasflags.forceWindowed)
//		return DD_OK;
	return m_dd->StartModeTest(a,b,c);
}
template<> HRESULT MyDirectDraw<IDirectDraw7>::EvaluateMode(DWORD a, DWORD* b)
{
	ddrawdebugprintf(__FUNCTION__ " called.\n");
//	if(tasflags.forceWindowed)
//		return DD_OK;
	return m_dd->EvaluateMode(a,b);
}

template<> const GUID MyDirectDraw<IDirectDraw>::IID_IDirectDrawSurfaceN = IID_IDirectDrawSurface;
template<> const GUID MyDirectDraw<IDirectDraw2>::IID_IDirectDrawSurfaceN = IID_IDirectDrawSurface;
template<> const GUID MyDirectDraw<IDirectDraw4>::IID_IDirectDrawSurfaceN = IID_IDirectDrawSurface4;
template<> const GUID MyDirectDraw<IDirectDraw7>::IID_IDirectDrawSurfaceN = IID_IDirectDrawSurface7;







bool g_gammaRampEnabled = false;
DDGAMMARAMP g_gammaRamp;

DEFINE_LOCAL_GUID(IID_IDirectDrawGammaControl, 0x69C11C3E,0xB46B,0x11D1,0xAD,0x7A,0x00,0xC0,0x4F,0xC2,0x9B,0x4E);
struct MyDirectDrawGammaControl
{
	static BOOL Hook(IDirectDrawGammaControl* obj)
	{
		BOOL rv = FALSE;
		rv |= VTHOOKFUNC(IDirectDrawGammaControl, GetGammaRamp);
		rv |= VTHOOKFUNC(IDirectDrawGammaControl, SetGammaRamp);
		rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
		return rv;
	}

    static HRESULT(STDMETHODCALLTYPE *GetGammaRamp)(IDirectDrawGammaControl* pThis, DWORD dwFlags, LPDDGAMMARAMP pRamp);
    static HRESULT STDMETHODCALLTYPE MyGetGammaRamp(IDirectDrawGammaControl* pThis, DWORD dwFlags, LPDDGAMMARAMP pRamp)
	{
		ddrawdebugprintf(__FUNCTION__ " called.\n");
		//HRESULT rv = GetGammaRamp(pThis, dwFlags, pRamp);
		HRESULT rv;
		if(pRamp)
		{
			WORD* dstReds = pRamp->red;
			WORD* dstGreens = pRamp->green;
			WORD* dstBlues = pRamp->blue;
			if(g_gammaRampEnabled)
			{
				for(int i = 0; i < 256; i++) dstReds[i] = g_gammaRamp.red[i];
				for(int i = 0; i < 256; i++) dstGreens[i] = g_gammaRamp.green[i];
				for(int i = 0; i < 256; i++) dstBlues[i] = g_gammaRamp.blue[i];
			}
			else
			{
				for(int i = 0; i < 256; i++) dstReds[i] = i|(i<<8);
				for(int i = 0; i < 256; i++) dstGreens[i] = i|(i<<8);
				for(int i = 0; i < 256; i++) dstBlues[i] = i|(i<<8);
			}
			rv = DD_OK;
		}
		else
		{
			rv = E_POINTER;
		}
		//for(int i = 0; i < 256; i++)
		//	debugprintf("ramp[%d] = %d,%d,%d\n", i, pRamp->red[i],pRamp->green[i],pRamp->blue[i]);
		return rv;
	}

    static HRESULT(STDMETHODCALLTYPE *SetGammaRamp)(IDirectDrawGammaControl* pThis, DWORD dwFlags, LPDDGAMMARAMP pRamp);
    static HRESULT STDMETHODCALLTYPE MySetGammaRamp(IDirectDrawGammaControl* pThis, DWORD dwFlags, LPDDGAMMARAMP pRamp)
	{
		ddrawdebugprintf(__FUNCTION__ " called (%d/65536).\n", pRamp ? pRamp->red[255] : 0);
		//for(int i = 0; i < 256; i++)
		//	debugprintf("ramp[%d] = %d,%d,%d\n", i, pRamp->red[i],pRamp->green[i],pRamp->blue[i]);
		//HRESULT rv = SetGammaRamp(pThis, dwFlags, pRamp);
		HRESULT rv;
		if(pRamp)
		{
			WORD* srcReds = pRamp->red;
			WORD* srcGreens = pRamp->green;
			WORD* srcBlues = pRamp->blue;
			for(int i = 0; i < 256; i++) g_gammaRamp.red[i] = srcReds[i];
			for(int i = 0; i < 256; i++) g_gammaRamp.green[i] = srcGreens[i];
			for(int i = 0; i < 256; i++) g_gammaRamp.blue[i] = srcBlues[i];
			g_gammaRampEnabled = true;
			if(g_gammaRamp.red[255] == 65535 && g_gammaRamp.green[255] == 65535 && g_gammaRamp.blue[255] == 65535)
				g_gammaRampEnabled = false; // optimization
			cmdprintf("GAMMARAMPDATA: %Iu", g_gammaRampEnabled ? &g_gammaRamp : 0);
			rv = DD_OK;
		}
		else
		{
			rv = E_POINTER;
		}
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IDirectDrawGammaControl* pThis, REFIID riid, void** ppvObj);
	static HRESULT STDMETHODCALLTYPE MyQueryInterface(IDirectDrawGammaControl* pThis, REFIID riid, void** ppvObj)
	{
		ddrawdebugprintf(__FUNCTION__ " called (0x%X).\n", riid.Data1);
		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}
};

#define DEF(x) HRESULT (STDMETHODCALLTYPE* MyDirectDrawGammaControl::QueryInterface)(x* pThis, REFIID riid, void** ppvObj) = 0; \
               HRESULT (STDMETHODCALLTYPE* MyDirectDrawGammaControl::GetGammaRamp)(x* pThis, DWORD dwFlags, LPDDGAMMARAMP pRamp) = 0; \
               HRESULT (STDMETHODCALLTYPE* MyDirectDrawGammaControl::SetGammaRamp)(x* pThis, DWORD dwFlags, LPDDGAMMARAMP pRamp) = 0;
	DEF(IDirectDrawGammaControl)
#undef DEF


HOOKFUNC HRESULT WINAPI MyDirectDrawCreate(GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter)
{
	debuglog(LCF_DDRAW, __FUNCTION__ " called.\n");
	ThreadLocalStuff& curtls = tls;
	curtls.callerisuntrusted++;
	//debugprintf("lplpDD = 0x%X\n", lplpDD);
	HRESULT rv = DirectDrawCreate(lpGUID, lplpDD, pUnkOuter);

	if(SUCCEEDED(rv))
		HookCOMInterface(IID_IDirectDraw, (LPVOID*)lplpDD);
	else
		debuglog(LCF_DDRAW|LCF_ERROR, "DirectDrawCreate FAILED, all on its own.\n");
	curtls.callerisuntrusted--;

	return rv;
}

HOOKFUNC HRESULT WINAPI MyDirectDrawCreateEx(GUID FAR * lpGuid, LPVOID  *lplpDD, REFIID riid,IUnknown FAR *pUnkOuter)
{
	debuglog(LCF_DDRAW, __FUNCTION__ "(0x%X) called.\n", riid.Data1);
	ThreadLocalStuff& curtls = tls;
	curtls.callerisuntrusted++;
	HRESULT rv = DirectDrawCreateEx(lpGuid, lplpDD, riid, pUnkOuter);

	if(SUCCEEDED(rv))
		HookCOMInterface(riid, lplpDD);
	else
		debuglog(LCF_DDRAW|LCF_ERROR, "DirectDrawCreateEx FAILED, all on its own.\n");
	curtls.callerisuntrusted--;

	return rv;
}

HOOKFUNC HRESULT WINAPI MyCheckFullscreen()
{
	debuglog(LCF_DDRAW|LCF_UNTESTED|LCF_TODO, __FUNCTION__ " called.\n");
	//return CheckFullscreen();
	return DD_OK;
}


bool HookCOMInterfaceDDraw(REFIID riid, LPVOID* ppvOut, bool uncheckedFastNew)
{
	if(!ppvOut)
		return true;

	switch(riid.Data1)
	{
		//VTHOOKRIID(DirectDraw,);
		//VTHOOKRIID(DirectDraw,2);
		//VTHOOKRIID(DirectDraw,4);
		//VTHOOKRIID(DirectDraw,7); // can't use vt hooking with IDirectDraw7 (fails in BMD because CoCreateInstance with CLSID_DirectDraw7 gives us something screwy that overwrites its vtable or something and thus can't be hooked, and replacing it would break alpha blending)
		HOOKRIID(DirectDraw,);
		HOOKRIID(DirectDraw,2);
		HOOKRIID(DirectDraw,4);
		HOOKRIID(DirectDraw,7);

		VTHOOKRIID(DirectDrawSurface,);
		VTHOOKRIID(DirectDrawSurface,2);
		VTHOOKRIID(DirectDrawSurface,3);
		VTHOOKRIID(DirectDrawSurface,4);
		VTHOOKRIID(DirectDrawSurface,7);

		VTHOOKRIID3(IDirectDrawGammaControl,MyDirectDrawGammaControl);

		default: return false;
	}

	return true;
}

void ApplyDDrawIntercepts()
{
	static const InterceptDescriptor intercepts [] = 
	{
		MAKE_INTERCEPT(1, DDRAW, DirectDrawCreate),
		MAKE_INTERCEPT(1, DDRAW, DirectDrawCreateEx),
		MAKE_INTERCEPT(1, DDRAW, CheckFullscreen),
	};
	ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
}

#else
#pragma message(__FILE__": (skipped compilation)")
#endif

/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(D3D8HOOKS_INCL) && !defined(UNITY_BUILD)
#define D3D8HOOKS_INCL

#include "../../external/d3d8.h"
#include "../global.h"
#include "../wintasee.h"
#include "../tls.h"
#include <map>

//#define SAVESTATE_DX8_TEXTURES

void FakeBroadcastDisplayChange(int width, int height, int depth);

DEFINE_LOCAL_GUID(IID_IDirect3D8,0x1DD9E8DA,0x1C77,0x4D40,0xB0,0xCF,0x98,0xFE,0xFD,0xFF,0x95,0x12);
DEFINE_LOCAL_GUID(IID_IDirect3DDevice8,0x7385E5DF,0x8FE8,0x41D5,0x86,0xB6,0xD7,0xB4,0x85,0x47,0xB6,0xCF);
DEFINE_LOCAL_GUID(IID_IDirect3DSwapChain8,0x928C088B,0x76B9,0x4C6B,0xA5,0x36,0xA5,0x90,0x85,0x38,0x76,0xCD);
DEFINE_LOCAL_GUID(IID_IDirect3DSurface8,0xB96EEBCA,0xB326,0x4EA5,0x88,0x2F,0x2F,0xF5,0xBA,0xE0,0x21,0xDD);
DEFINE_LOCAL_GUID(IID_IDirect3DTexture8,0xE4CDD575,0x2866,0x4F01,0xB1,0x2E,0x7E,0xEC,0xE1,0xEC,0x93,0x58);

static IDirect3DDevice8* pBackBufCopyOwner = NULL;
static IDirect3DSurface8* pBackBufCopy = NULL;

static IDirect3DDevice8* s_saved_d3d8Device = NULL;
static IDirect3DSwapChain8* s_saved_d3d8SwapChain = NULL;
static RECT s_savedD3D8SrcRect = {};
static RECT s_savedD3D8DstRect = {};
static LPRECT s_savedD3D8pSrcRect = NULL;
static LPRECT s_savedD3D8pDstRect = NULL;
static HWND s_savedD3D8HWND = NULL;
static HWND s_savedD3D8DefaultHWND = NULL;
static RECT s_savedD3D8ClientRect = {};

std::map<IDirect3DSwapChain8*,IDirect3DDevice8*> d3d8SwapChainToDeviceMap;

static bool d3d8BackBufActive = true;
static bool d3d8BackBufDirty = true;

// I feel like there must be some smarter way of storing this custom data than with maps,
// but wrapping the interfaces breaks some internal code in d3d that assumes the interface can be cast to a specific internal type,
// and without wrapping the interface I'm not sure how else I could add my own data to the class.

struct IDirect3DSurface8_CustomData
{
	void* videoMemoryPixelBackup;
	bool videoMemoryBackupDirty;
	bool ownedByTexture;
	bool isBackBuffer;
};
static std::map<IDirect3DSurface8*, IDirect3DSurface8_CustomData> surface8data;

struct IDirect3DTexture8_CustomData
{
	bool valid;
	bool dirty;
};
static std::map<IDirect3DTexture8*, IDirect3DTexture8_CustomData> texture8data;


static void ProcessPresentationParams8(D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3D8* d3d, IDirect3DDevice8* d3dDevice)
{
	if(pPresentationParameters) // presentparams
	{
		pPresentationParameters->Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER; // back buffer must be lockable, to allow for AVI recording (TODO: maybe that's not true anymore except if the usual method fails... if there's a significant performance penalty for this flag (which there might not be) then there should be an option to disable it)
		//wasWindowed = pPresentationParameters->Windowed;
		if(tasflags.forceWindowed && !pPresentationParameters->Windowed)
		{
			pPresentationParameters->Windowed = TRUE;
			pPresentationParameters->FullScreen_RefreshRateInHz = 0;
			pPresentationParameters->FullScreen_PresentationInterval = 0;

			fakeDisplayWidth = pPresentationParameters->BackBufferWidth;
			fakeDisplayHeight = pPresentationParameters->BackBufferHeight;
			fakePixelFormatBPP = (pPresentationParameters->BackBufferFormat == D3DFMT_R5G6B5 || pPresentationParameters->BackBufferFormat == D3DFMT_X1R5G5B5 || pPresentationParameters->BackBufferFormat == D3DFMT_A1R5G5B5) ? 16 : 32;
			fakeDisplayValid = TRUE;
			FakeBroadcastDisplayChange(fakeDisplayWidth,fakeDisplayHeight,fakePixelFormatBPP);
			if(gamehwnd)
				MakeWindowWindowed(gamehwnd, fakeDisplayWidth, fakeDisplayHeight);

			D3DDISPLAYMODE display_mode;
			if(d3d)
				d3dDevice = NULL;
			if(!d3d && d3dDevice)
				d3dDevice->GetDirect3D(&d3d);

			if(SUCCEEDED(d3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &display_mode)))
				pPresentationParameters->BackBufferFormat = display_mode.Format;

			if(d3d && d3dDevice)
				d3d->Release();

			pPresentationParameters->BackBufferCount = 1;
		}
		else if(!tasflags.forceWindowed && !pPresentationParameters->Windowed)
		{
			FakeBroadcastDisplayChange(pPresentationParameters->BackBufferWidth, pPresentationParameters->BackBufferHeight, (pPresentationParameters->BackBufferFormat == D3DFMT_R5G6B5 || pPresentationParameters->BackBufferFormat == D3DFMT_X1R5G5B5 || pPresentationParameters->BackBufferFormat == D3DFMT_A1R5G5B5) ? 16 : 32);
		}
	}
}


struct MyDirect3DDevice8
{
	static BOOL Hook(IDirect3DDevice8* obj)
	{
		BOOL rv = FALSE;
		rv |= VTHOOKFUNC(IDirect3DDevice8, Present);
		rv |= VTHOOKFUNC(IDirect3DDevice8, Reset);
		rv |= VTHOOKFUNC(IDirect3DDevice8, CreateAdditionalSwapChain);
		rv |= VTHOOKFUNC(IDirect3DDevice8, SetRenderTarget);
		rv |= VTHOOKFUNC(IDirect3DDevice8, GetRenderTarget);

		rv |= VTHOOKFUNC(IDirect3DDevice8, CopyRects);
		//rv |= VTHOOKFUNC(IDirect3DDevice8, UpdateTexture);
		rv |= VTHOOKFUNC(IDirect3DDevice8, Clear);
		rv |= VTHOOKFUNC(IDirect3DDevice8, DrawPrimitive);
		rv |= VTHOOKFUNC(IDirect3DDevice8, DrawIndexedPrimitive);
		rv |= VTHOOKFUNC(IDirect3DDevice8, DrawPrimitiveUP);
		rv |= VTHOOKFUNC(IDirect3DDevice8, DrawIndexedPrimitiveUP);
		//rv |= VTHOOKFUNC(IDirect3DDevice8, DrawRectPatch);
		//rv |= VTHOOKFUNC(IDirect3DDevice8, DrawTriPatch);
		rv |= VTHOOKFUNC(IDirect3DDevice8, Release);
		rv |= VTHOOKFUNC(IDirect3DDevice8, CreateTexture);
		rv |= VTHOOKFUNC(IDirect3DDevice8, CreateRenderTarget);
		//rv |= VTHOOKFUNC(IDirect3DDevice8, CreateImageSurface);
	
	

		rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
		return rv;
	}

	static ULONG(STDMETHODCALLTYPE *Release)(IDirect3DDevice8* pThis);
	static ULONG STDMETHODCALLTYPE MyRelease(IDirect3DDevice8* pThis)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
		ULONG rv = Release(pThis);
		if(rv == 0)
		{
			if(pBackBufCopyOwner == pThis) { pBackBufCopyOwner = NULL; }
			if(s_saved_d3d8Device == pThis) { s_saved_d3d8Device = NULL; }
		}
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IDirect3DDevice8* pThis, REFIID riid, void** ppvObj);
	static HRESULT STDMETHODCALLTYPE MyQueryInterface(IDirect3DDevice8* pThis, REFIID riid, void** ppvObj)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *Reset)(IDirect3DDevice8* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters);
	static HRESULT STDMETHODCALLTYPE MyReset(IDirect3DDevice8* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		ProcessPresentationParams8(pPresentationParameters, NULL, pThis);
		d3d8BackBufActive = true;
		d3d8BackBufDirty = true;
		HRESULT rv = Reset(pThis, pPresentationParameters);
		return rv;
	}

	static void PresentFrameBoundary(IDirect3DDevice8* pThis, CONST RECT* pSourceRect, CONST RECT* pDestRect)
	{
		// are we (not) recording AVI?
		BOOL recordingAVIVideo = (tasflags.aviMode & 1);
		if(!recordingAVIVideo)
		{
			// if not recording AVI, it's a regular frame boundary.
			FrameBoundary(NULL, CAPTUREINFO_TYPE_NONE);
		}
		else
		{
			// if we are, it's still a regular frame boundary,
			// but we prepare extra info for the AVI capture around it.
			DDSURFACEDESC desc = { sizeof(DDSURFACEDESC) };
			IDirect3DSurface8* pBackBuffer = NULL;
#if 0 // slow
			Lock(pThis, desc, pBackBuffer, pSourceRect);
			FrameBoundary(&desc, CAPTUREINFO_TYPE_DDSD);
			pBackBuffer->UnlockRect();
#else // MUCH faster.
	#ifdef _DEBUG
			DWORD time1 = timeGetTime();
	#endif
			pThis->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&pBackBuffer);
			if(pBackBufCopyOwner != pThis /*&& pBackBufCopy*/)
			{
				//pBackBufCopy->Release();
				pBackBufCopy = NULL;
			}
			IDirect3DSurface8* pSurface = pBackBufCopy;
			if(!pSurface)
			{
				D3DSURFACE_DESC d3ddesc;
				pBackBuffer->GetDesc(&d3ddesc);
				if(SUCCEEDED(pThis->CreateImageSurface(d3ddesc.Width, d3ddesc.Height, d3ddesc.Format, &pBackBufCopy)))
				{
					pSurface = pBackBufCopy;
					pBackBufCopyOwner = pThis;
				}
				else
				{
					// fallback
					pSurface = pBackBuffer;
				}
			}
			if(pSurface != pBackBuffer)
				if(FAILED(CopyRects(pThis, pBackBuffer,pSourceRect,pSourceRect?1:0,pSurface,NULL)))
					pSurface = pBackBuffer;
			if(desc.ddpfPixelFormat.dwRGBBitCount == 8)
				pThis->GetPaletteEntries(0, &activePalette[0]);
			Lock(pThis, desc, pSurface, pSourceRect, false);
	#ifdef _DEBUG
			DWORD time2 = timeGetTime();
			debugprintf("AVI: pre-copying pixel data took %d ticks\n", (int)(time2-time1));
	#endif
			FrameBoundary(&desc, CAPTUREINFO_TYPE_DDSD);
			pSurface->UnlockRect();
			if(pBackBuffer)
				pBackBuffer->Release();
#endif
		}
	}

	static HRESULT(STDMETHODCALLTYPE *Present)(IDirect3DDevice8* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion);
	static HRESULT STDMETHODCALLTYPE MyPresent(IDirect3DDevice8* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");

		HRESULT rv;
		if(ShouldSkipDrawing(d3d8BackBufDirty, !d3d8BackBufDirty))
			rv = D3D_OK;
		else
			rv = Present(pThis, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);


		if((d3d8BackBufActive || d3d8BackBufDirty) && !redrawingScreen)
		{
			s_saved_d3d8SwapChain = NULL;
			s_saved_d3d8Device = pThis;
			s_savedD3D8pSrcRect = pSourceRect ? &s_savedD3D8SrcRect : NULL;
			s_savedD3D8pDstRect = pDestRect ? &s_savedD3D8DstRect : NULL;
			if(pSourceRect) s_savedD3D8SrcRect = *pSourceRect;
			if(pDestRect) s_savedD3D8DstRect = *pDestRect;
			s_savedD3D8HWND = hDestWindowOverride ? hDestWindowOverride : s_savedD3D8DefaultHWND;
			GetClientRect(s_savedD3D8HWND, &s_savedD3D8ClientRect);

			PresentFrameBoundary(pThis, pSourceRect, pDestRect);

			d3d8BackBufDirty = false;
		}

		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *CreateAdditionalSwapChain)(IDirect3DDevice8* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain8** pSwapChain);
	static HRESULT STDMETHODCALLTYPE MyCreateAdditionalSwapChain(IDirect3DDevice8* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain8** pSwapChain)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		ProcessPresentationParams8(pPresentationParameters, NULL, pThis);
		HRESULT rv = CreateAdditionalSwapChain(pThis, pPresentationParameters, pSwapChain);
		if(SUCCEEDED(rv))
		{
			HookCOMInterface(IID_IDirect3DSwapChain8, (LPVOID*)pSwapChain);
			if(pSwapChain)
				d3d8SwapChainToDeviceMap[*pSwapChain] = pThis;
		}
		return rv;
	}

	// DrawPrimitive
	//{
	//	if(ShouldSkipDrawing(false, true))
	//		return D3D_OK;
	//}

	static HRESULT(STDMETHODCALLTYPE *SetRenderTarget)(IDirect3DDevice8* pThis, IDirect3DSurface8* pRenderTarget,IDirect3DSurface8* pNewZStencil);
	static HRESULT STDMETHODCALLTYPE MySetRenderTarget(IDirect3DDevice8* pThis, IDirect3DSurface8* pRenderTarget,IDirect3DSurface8* pNewZStencil)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pRenderTarget);
		HRESULT rv = SetRenderTarget(pThis, pRenderTarget, pNewZStencil);
		IDirect3DSurface8* pBackBuffer;
		if(SUCCEEDED(pThis->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&pBackBuffer)))
			if(pBackBuffer->Release() != 0)
				d3d8BackBufActive = (pRenderTarget == pBackBuffer);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *GetRenderTarget)(IDirect3DDevice8* pThis, IDirect3DSurface8** ppRenderTarget);
	static HRESULT STDMETHODCALLTYPE MyGetRenderTarget(IDirect3DDevice8* pThis, IDirect3DSurface8** ppRenderTarget)
	{
		HRESULT rv = GetRenderTarget(pThis, ppRenderTarget);
		d3ddebugprintf(__FUNCTION__ "(0x%X) called, returned 0x%X\n", ppRenderTarget, (SUCCEEDED(rv) && ppRenderTarget) ? *ppRenderTarget : NULL);
		return rv;
	}

	static void Lock(IDirect3DDevice8* pThis, DDSURFACEDESC& desc, IDirect3DSurface8*& pBackBuffer, CONST RECT* pSourceRect=NULL, bool getBackBuffer=true)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		if(getBackBuffer)
			pThis->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&pBackBuffer);

		D3DSURFACE_DESC d3ddesc;
		pBackBuffer->GetDesc(&d3ddesc);

		D3DLOCKED_RECT lr;
		pBackBuffer->LockRect(&lr,0,D3DLOCK_NOSYSLOCK|D3DLOCK_READONLY|D3DLOCK_NO_DIRTY_UPDATE);

		desc.lpSurface = lr.pBits;
		desc.dwWidth = d3ddesc.Width;
		desc.dwHeight = d3ddesc.Height;
		desc.lPitch = lr.Pitch;
		switch(d3ddesc.Format)
		{
		default:
		case D3DFMT_A8R8G8B8:
		case D3DFMT_X8R8G8B8:
			desc.ddpfPixelFormat.dwRGBBitCount = 32;
			desc.ddpfPixelFormat.dwRBitMask = 0x00FF0000;
			desc.ddpfPixelFormat.dwGBitMask = 0x0000FF00;
			desc.ddpfPixelFormat.dwBBitMask = 0x000000FF;
			break;
		case D3DFMT_A1R5G5B5:
		case D3DFMT_X1R5G5B5:
			desc.ddpfPixelFormat.dwRGBBitCount = 16;
			desc.ddpfPixelFormat.dwRBitMask = 0x7C00;
			desc.ddpfPixelFormat.dwGBitMask = 0x03E0;
			desc.ddpfPixelFormat.dwBBitMask = 0x001F;
			break;
		case D3DFMT_R5G6B5:
			desc.ddpfPixelFormat.dwRGBBitCount = 16;
			desc.ddpfPixelFormat.dwRBitMask = 0xF800;
			desc.ddpfPixelFormat.dwGBitMask = 0x07E0;
			desc.ddpfPixelFormat.dwBBitMask = 0x001F;
			break;
		//case D3DFMT_A2R10G10B10:
		}

		if(pSourceRect)
		{
			desc.dwWidth = min(desc.dwWidth, (DWORD)(pSourceRect->right - pSourceRect->left));
			desc.dwHeight = min(desc.dwHeight, (DWORD)(pSourceRect->bottom - pSourceRect->top));
		}

		if(getBackBuffer)
			pBackBuffer->Release();
	}

    static HRESULT(STDMETHODCALLTYPE *CopyRects)(IDirect3DDevice8* pThis, IDirect3DSurface8* pSourceSurface,CONST RECT* pSourceRectsArray,UINT cRects,IDirect3DSurface8* pDestinationSurface,CONST POINT* pDestPointsArray);
    static HRESULT STDMETHODCALLTYPE MyCopyRects(IDirect3DDevice8* pThis, IDirect3DSurface8* pSourceSurface,CONST RECT* pSourceRectsArray,UINT cRects,IDirect3DSurface8* pDestinationSurface,CONST POINT* pDestPointsArray)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		HRESULT rv = CopyRects(pThis, pSourceSurface,pSourceRectsArray,cRects,pDestinationSurface,pDestPointsArray);
		IDirect3DSurface8* pBuffer;
		if(!redrawingScreen)
		{
			if(!d3d8BackBufDirty && SUCCEEDED(pThis->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&pBuffer)))
			{
				if(pDestinationSurface == pBuffer)
					d3d8BackBufDirty = true;
				pBuffer->Release();
			}
			// NYI. maybe need to call PresentFrameBoundary in some cases (if pDestinationSurface can be the front buffer)
			//else if(SUCCEEDED(pThis->GetFrontBuffer(&pBuffer)) && pDestinationSurface == pBuffer)
			//	PresentFrameBoundary(pThis);
		}
		return rv;
	}

    static HRESULT(STDMETHODCALLTYPE *Clear)(IDirect3DDevice8* pThis, DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil);
    static HRESULT STDMETHODCALLTYPE MyClear(IDirect3DDevice8* pThis, DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = Clear(pThis, Count,pRects,Flags,Color,Z,Stencil);
		if(!d3d8BackBufDirty && d3d8BackBufActive)
			d3d8BackBufDirty = true;
		return rv;
	}

    static HRESULT(STDMETHODCALLTYPE *DrawPrimitive)(IDirect3DDevice8* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount);
    static HRESULT STDMETHODCALLTYPE MyDrawPrimitive(IDirect3DDevice8* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = DrawPrimitive(pThis, PrimitiveType,StartVertex,PrimitiveCount);
		if(!d3d8BackBufDirty && d3d8BackBufActive)
			d3d8BackBufDirty = true;
		return rv;
	}

    static HRESULT(STDMETHODCALLTYPE *DrawIndexedPrimitive)(IDirect3DDevice8* pThis, D3DPRIMITIVETYPE primitiveType,UINT minIndex,UINT NumVertices,UINT startIndex,UINT primCount);
    static HRESULT STDMETHODCALLTYPE MyDrawIndexedPrimitive(IDirect3DDevice8* pThis, D3DPRIMITIVETYPE primitiveType,UINT minIndex,UINT NumVertices,UINT startIndex,UINT primCount)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = DrawIndexedPrimitive(pThis, primitiveType,minIndex,NumVertices,startIndex,primCount);
		if(!d3d8BackBufDirty && d3d8BackBufActive)
			d3d8BackBufDirty = true;
		return rv;
	}

    static HRESULT(STDMETHODCALLTYPE *DrawPrimitiveUP)(IDirect3DDevice8* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride);
    static HRESULT STDMETHODCALLTYPE MyDrawPrimitiveUP(IDirect3DDevice8* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = DrawPrimitiveUP(pThis, PrimitiveType,PrimitiveCount,pVertexStreamZeroData,VertexStreamZeroStride);
		if(!d3d8BackBufDirty && d3d8BackBufActive)
			d3d8BackBufDirty = true;
		return rv;
	}

    static HRESULT(STDMETHODCALLTYPE *DrawIndexedPrimitiveUP)(IDirect3DDevice8* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertexIndices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride);
    static HRESULT STDMETHODCALLTYPE MyDrawIndexedPrimitiveUP(IDirect3DDevice8* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertexIndices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = DrawIndexedPrimitiveUP(pThis, PrimitiveType,MinVertexIndex,NumVertexIndices,PrimitiveCount,pIndexData,IndexDataFormat,pVertexStreamZeroData,VertexStreamZeroStride);
		if(!d3d8BackBufDirty && d3d8BackBufActive)
			d3d8BackBufDirty = true;
		return rv;
	}




	static HRESULT(STDMETHODCALLTYPE *CreateTexture)(IDirect3DDevice8* pThis, UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture8** ppTexture);
	static HRESULT STDMETHODCALLTYPE MyCreateTexture(IDirect3DDevice8* pThis, UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture8** ppTexture)
	{
		d3ddebugprintf(__FUNCTION__ "(%dx%d, pool=%d) called.\n", Width, Height, Pool);
		HRESULT rv = CreateTexture(pThis,Width,Height,Levels,Usage,Format,Pool,ppTexture);
		if(SUCCEEDED(rv))
		{
			HookCOMInterface(IID_IDirect3DTexture8, reinterpret_cast<LPVOID*>(ppTexture));

			IDirect3DTexture8* pTexture = *ppTexture;
			int numLevels = pTexture->GetLevelCount();
			for(int i = 0; i < numLevels; i++)
			{
				IDirect3DSurface8* pSurface = NULL;
				if(SUCCEEDED(pTexture->GetSurfaceLevel(i, &pSurface)))
				{
					HookCOMInterface(IID_IDirect3DSurface8, reinterpret_cast<LPVOID*>(&pSurface));
					IDirect3DSurface8_CustomData& surf8 = surface8data[pSurface];
					surf8.ownedByTexture = true;
					pSurface->Release();
				}
			}
		}
		return rv;
	}
	static HRESULT(STDMETHODCALLTYPE *CreateRenderTarget)(IDirect3DDevice8* pThis, UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,BOOL Lockable,IDirect3DSurface8** ppSurface);
	static HRESULT STDMETHODCALLTYPE MyCreateRenderTarget(IDirect3DDevice8* pThis, UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,BOOL Lockable,IDirect3DSurface8** ppSurface)
	{
		d3ddebugprintf(__FUNCTION__ "(%dx%d) called.\n", Width, Height);
		Lockable = TRUE;
		HRESULT rv = CreateRenderTarget(pThis,Width,Height,Format,MultiSample,Lockable,ppSurface);
		if(SUCCEEDED(rv))
		{
			HookCOMInterface(IID_IDirect3DSurface8, reinterpret_cast<LPVOID*>(ppSurface));
			IDirect3DSurface8_CustomData& surf8 = surface8data[*ppSurface];
			surf8.ownedByTexture = false;
		}
		return rv;
	}
	//static HRESULT(STDMETHODCALLTYPE *CreateImageSurface)(IDirect3DDevice8* pThis, UINT Width,UINT Height,D3DFORMAT Format,IDirect3DSurface8** ppSurface);
	//static HRESULT STDMETHODCALLTYPE MyCreateImageSurface(IDirect3DDevice8* pThis, UINT Width,UINT Height,D3DFORMAT Format,IDirect3DSurface8** ppSurface)
	//{
	//	d3ddebugprintf(__FUNCTION__ " called.\n");
	//	HRESULT rv = CreateImageSurface(pThis,Width,Height,Format,ppSurface);
	//	return rv;
	//}



};

HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::QueryInterface)(IDirect3DDevice8* pThis, REFIID riid, void** ppvObj) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::Present)(IDirect3DDevice8* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::Reset)(IDirect3DDevice8* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::CreateAdditionalSwapChain)(IDirect3DDevice8* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain8** pSwapChain) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::SetRenderTarget)(IDirect3DDevice8* pThis, IDirect3DSurface8* pRenderTarget,IDirect3DSurface8* pNewZStencil) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::GetRenderTarget)(IDirect3DDevice8* pThis, IDirect3DSurface8** ppRenderTarget) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::CopyRects)(IDirect3DDevice8* pThis, IDirect3DSurface8* pSourceSurface,CONST RECT* pSourceRectsArray,UINT cRects,IDirect3DSurface8* pDestinationSurface,CONST POINT* pDestPointsArray) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::DrawIndexedPrimitive)(IDirect3DDevice8* pThis, D3DPRIMITIVETYPE primitiveType,UINT minIndex,UINT NumVertices,UINT startIndex,UINT primCount) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::Clear)(IDirect3DDevice8* pThis, DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::DrawPrimitive)(IDirect3DDevice8* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::DrawPrimitiveUP)(IDirect3DDevice8* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::DrawIndexedPrimitiveUP)(IDirect3DDevice8* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertexIndices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride) = 0;
ULONG (STDMETHODCALLTYPE* MyDirect3DDevice8::Release)(IDirect3DDevice8* pThis) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::CreateTexture)(IDirect3DDevice8* pThis, UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture8** ppTexture) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice8::CreateRenderTarget)(IDirect3DDevice8* pThis, UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,BOOL Lockable,IDirect3DSurface8** ppSurface) = 0;



struct MyDirect3DSwapChain8
{
	static BOOL Hook(IDirect3DSwapChain8* obj)
	{
		BOOL rv = FALSE;
		rv |= VTHOOKFUNC(IDirect3DSwapChain8, Present);
		rv |= VTHOOKFUNC(IDirect3DSwapChain8, Release);
		rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
		return rv;
	}

	static ULONG(STDMETHODCALLTYPE *Release)(IDirect3DSwapChain8* pThis);
	static ULONG STDMETHODCALLTYPE MyRelease(IDirect3DSwapChain8* pThis)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
		ULONG rv = Release(pThis);
		if(rv == 0)
		{
			if(s_saved_d3d8SwapChain == pThis) { s_saved_d3d8SwapChain = NULL; }
		}
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IDirect3DSwapChain8* pThis, REFIID riid, void** ppvObj);
	static HRESULT STDMETHODCALLTYPE MyQueryInterface(IDirect3DSwapChain8* pThis, REFIID riid, void** ppvObj)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *Present)(IDirect3DSwapChain8* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion);
	static HRESULT STDMETHODCALLTYPE MyPresent(IDirect3DSwapChain8* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");

		HRESULT rv;
		if(ShouldSkipDrawing(true, false))
			rv = D3D_OK;
		else
			rv = Present(pThis, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);

		IDirect3DDevice8* pDevice;
		//if(SUCCEEDED(pThis->GetDevice(&pDevice)))
		if(0 != (pDevice = d3d8SwapChainToDeviceMap[pThis]) && !redrawingScreen)
		{
			s_saved_d3d8SwapChain = pThis;
			s_saved_d3d8Device = pDevice;
			s_savedD3D8pSrcRect = pSourceRect ? &s_savedD3D8SrcRect : NULL;
			s_savedD3D8pDstRect = pDestRect ? &s_savedD3D8DstRect : NULL;
			if(pSourceRect) s_savedD3D8SrcRect = *pSourceRect;
			if(pDestRect) s_savedD3D8DstRect = *pDestRect;
			s_savedD3D8HWND = hDestWindowOverride ? hDestWindowOverride : s_savedD3D8DefaultHWND;
			GetClientRect(s_savedD3D8HWND, &s_savedD3D8ClientRect);

			MyDirect3DDevice8::PresentFrameBoundary(pDevice,pSourceRect,pDestRect);

			d3d8BackBufDirty = false;
		}
		return rv;
	}
};

HRESULT (STDMETHODCALLTYPE* MyDirect3DSwapChain8::QueryInterface)(IDirect3DSwapChain8* pThis, REFIID riid, void** ppvObj) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DSwapChain8::Present)(IDirect3DSwapChain8* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion) = 0;
ULONG (STDMETHODCALLTYPE* MyDirect3DSwapChain8::Release)(IDirect3DSwapChain8* pThis) = 0;


#ifdef SAVESTATE_DX8_TEXTURES

// surfaces are hooked in order to backup their video memory in savestates
template<int IMPL_ID>
struct MyDirect3DSurface8
{
	static void* sm_vtable;
	static BOOL Hook(IDirect3DSurface8* obj)
	{
		void* vtable = (void*)*(size_t*)obj;
		if(sm_vtable && sm_vtable != vtable)
			return 0; // try next IMPL_ID

		VTHOOKFUNC(IDirect3DSurface8, Release);
		VTHOOKFUNC(IDirect3DSurface8, UnlockRect);
		VTHOOKFUNC(IDirect3DSurface8, QueryInterface);
		VTHOOKFUNC(IDirect3DSurface8, FreePrivateData);
		VTHOOKFUNC(IDirect3DSurface8, SetPrivateData);

		if(sm_vtable == vtable)
			return -1; // already hooked

		sm_vtable = vtable;
		return 1; // hooked new
	}

	static ULONG (STDMETHODCALLTYPE *Release)(IDirect3DSurface8* pThis);
	static ULONG STDMETHODCALLTYPE MyRelease(IDirect3DSurface8* pThis)
	{
		ULONG rv = Release(pThis);
		d3ddebugprintf(__FUNCTION__ "(0x%X) called, refcount -> %d.\n", pThis, rv);
		// note: pThis is invalid to dereference now, if rv==0
		if(!rv)
		{
			IDirect3DSurface8_CustomData& surf8 = surface8data[pThis];
			if(!surf8.isBackBuffer) // back buffer's ref count goes to 0 every frame? not sure why
			{
				surf8.videoMemoryBackupDirty = FALSE;

				void*& pixels = surf8.videoMemoryPixelBackup;
				free(pixels);
				pixels = NULL;
			}
		}
		return rv;
	}


    static HRESULT(STDMETHODCALLTYPE *SetPrivateData)(IDirect3DSurface8* pThis, REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags);
    static HRESULT STDMETHODCALLTYPE MySetPrivateData(IDirect3DSurface8* pThis, REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags)
	{
		HRESULT hr = SetPrivateData(pThis,refguid,pData,SizeOfData,Flags);

		IDirect3DSurface8_CustomData& surf8 = surface8data[pThis];
		surf8.videoMemoryBackupDirty = TRUE;

		return hr;
	}

    static HRESULT(STDMETHODCALLTYPE *FreePrivateData)(IDirect3DSurface8* pThis, REFGUID refguid);
    static HRESULT STDMETHODCALLTYPE MyFreePrivateData(IDirect3DSurface8* pThis, REFGUID refguid)
	{
		HRESULT hr = FreePrivateData(pThis,refguid);

		IDirect3DSurface8_CustomData& surf8 = surface8data[pThis];
		surf8.videoMemoryBackupDirty = FALSE;
		void*& pixels = surf8.videoMemoryPixelBackup;
		free(pixels);
		pixels = NULL;

		return hr;
	}


	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IDirect3DSurface8* pThis, REFIID riid, void** ppvObj);
	static HRESULT STDMETHODCALLTYPE MyQueryInterface(IDirect3DSurface8* pThis, REFIID riid, void** ppvObj)
	{
		d3ddebugprintf(__FUNCTION__ " called (0x%X).\n", riid.Data1);
		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *UnlockRect)(IDirect3DSurface8* pThis);
	static HRESULT STDMETHODCALLTYPE MyUnlockRect(IDirect3DSurface8* pThis)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
		HRESULT rv = UnlockRect(pThis);
		IDirect3DSurface8_CustomData& surf8 = surface8data[pThis];
		surf8.videoMemoryBackupDirty = TRUE;
		return rv;
	}
};


#define HRESULT long // some versions of visual studio (including 2008) don't understand "typedef long HRESULT;" (the compiler claims HRESULT is an undefined type even after it's typedef'd to long)

#define DEF(x,IMPL_ID) \
	ULONG (STDMETHODCALLTYPE* MyDirect3DSurface8<IMPL_ID>::Release)(x* pThis) = 0; \
	HRESULT(STDMETHODCALLTYPE* MyDirect3DSurface8<IMPL_ID>::QueryInterface)(x* pThis, REFIID riid, void** ppvObj) = 0; \
    HRESULT(STDMETHODCALLTYPE* MyDirect3DSurface8<IMPL_ID>::UnlockRect)(x* pThis) = 0; \
    HRESULT(STDMETHODCALLTYPE* MyDirect3DSurface8<IMPL_ID>::SetPrivateData)(x* pThis, REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags) = 0; \
    HRESULT(STDMETHODCALLTYPE* MyDirect3DSurface8<IMPL_ID>::FreePrivateData)(x* pThis, REFGUID refguid) = 0; \
	void* MyDirect3DSurface8<IMPL_ID>::sm_vtable = 0;

	DEF(IDirect3DSurface8, 0)
	DEF(IDirect3DSurface8, 1)
	DEF(IDirect3DSurface8, 2)
#undef DEF

#undef HRESULT


struct MyDirect3DTexture8
{
	static BOOL Hook(IDirect3DTexture8* obj)
	{
		BOOL rv = FALSE;
		rv |= VTHOOKFUNC(IDirect3DTexture8, Release);
		rv |= VTHOOKFUNC(IDirect3DTexture8, UnlockRect);
		rv |= VTHOOKFUNC(IDirect3DTexture8, QueryInterface);
		rv |= VTHOOKFUNC(IDirect3DTexture8, FreePrivateData);
		rv |= VTHOOKFUNC(IDirect3DTexture8, SetPrivateData);

		IDirect3DTexture8_CustomData& tex8 = texture8data[obj];
		tex8.valid = true;

		return rv;
	}

	static ULONG (STDMETHODCALLTYPE *Release)(IDirect3DTexture8* pThis);
	static ULONG STDMETHODCALLTYPE MyRelease(IDirect3DTexture8* pThis)
	{
		ULONG rv = Release(pThis);
		d3ddebugprintf(__FUNCTION__ "(0x%X) called, refcount -> %d.\n", pThis, rv);
		// note: pThis is invalid to dereference now, if rv==0
		if(!rv)
		{
			IDirect3DTexture8_CustomData& tex8 = texture8data[pThis];
			tex8.valid = false;
		}
		return rv;
	}


    static HRESULT(STDMETHODCALLTYPE *SetPrivateData)(IDirect3DTexture8* pThis, REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags);
    static HRESULT STDMETHODCALLTYPE MySetPrivateData(IDirect3DTexture8* pThis, REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags)
	{
		HRESULT hr = SetPrivateData(pThis,refguid,pData,SizeOfData,Flags);
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);

		IDirect3DTexture8_CustomData& tex8 = texture8data[pThis];
		tex8.dirty = true;

		return hr;
	}

    static HRESULT(STDMETHODCALLTYPE *FreePrivateData)(IDirect3DTexture8* pThis, REFGUID refguid);
    static HRESULT STDMETHODCALLTYPE MyFreePrivateData(IDirect3DTexture8* pThis, REFGUID refguid)
	{
		HRESULT hr = FreePrivateData(pThis,refguid);
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);

		IDirect3DTexture8_CustomData& tex8 = texture8data[pThis];
		tex8.dirty = false;
		tex8.valid = false;

		return hr;
	}


	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IDirect3DTexture8* pThis, REFIID riid, void** ppvObj);
	static HRESULT STDMETHODCALLTYPE MyQueryInterface(IDirect3DTexture8* pThis, REFIID riid, void** ppvObj)
	{
		d3ddebugprintf(__FUNCTION__ " called (0x%X).\n", riid.Data1);
		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *UnlockRect)(IDirect3DTexture8* pThis, UINT Level);
	static HRESULT STDMETHODCALLTYPE MyUnlockRect(IDirect3DTexture8* pThis, UINT Level)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
		HRESULT rv = UnlockRect(pThis, Level);
		IDirect3DTexture8_CustomData& tex8 = texture8data[pThis];
		tex8.dirty = true;
		return rv;
	}
};

#define HRESULT long // some versions of visual studio (including 2008) don't understand "typedef long HRESULT;" (the compiler claims HRESULT is an undefined type even after it's typedef'd to long)

#define DEF(x) \
	ULONG (STDMETHODCALLTYPE* MyDirect3DTexture8::Release)(x* pThis) = 0; \
	HRESULT(STDMETHODCALLTYPE* MyDirect3DTexture8::QueryInterface)(x* pThis, REFIID riid, void** ppvObj) = 0; \
    HRESULT(STDMETHODCALLTYPE* MyDirect3DTexture8::UnlockRect)(x* pThis, UINT) = 0; \
    HRESULT(STDMETHODCALLTYPE* MyDirect3DTexture8::SetPrivateData)(x* pThis, REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags) = 0; \
    HRESULT(STDMETHODCALLTYPE* MyDirect3DTexture8::FreePrivateData)(x* pThis, REFGUID refguid) = 0;

	DEF(IDirect3DTexture8)
#undef DEF

#undef HRESULT


#endif // SAVESTATE_DX8_TEXTURES



static void BackupVideoMemory8(IDirect3DSurface8* pThis)
{
	IDirect3DSurface8_CustomData& surf8 = surface8data[pThis];
	//if(!surf8.videoMemoryBackupDirty)
	//	return;
	D3DSURFACE_DESC desc = {};
	if(SUCCEEDED(pThis->GetDesc(&desc)))
	{
		D3DLOCKED_RECT lockedRect;
		if(SUCCEEDED(pThis->LockRect(&lockedRect, NULL, D3DLOCK_NO_DIRTY_UPDATE|D3DLOCK_READONLY|D3DLOCK_NOSYSLOCK)))
		{
			int size = lockedRect.Pitch * desc.Height;
			void*& pixels = surf8.videoMemoryPixelBackup;
			pixels = realloc(pixels, size);
			memcpy(pixels, lockedRect.pBits, size);
			pThis->UnlockRect();
		}
	}
	surf8.videoMemoryBackupDirty = FALSE;
}

static void RestoreVideoMemory8(IDirect3DSurface8* pThis)
{
	IDirect3DSurface8_CustomData& surf8 = surface8data[pThis];
	if(surf8.videoMemoryBackupDirty)
		return;
	void*& pixels = surf8.videoMemoryPixelBackup;
	if(pixels)
	{
		D3DSURFACE_DESC desc = {};
		if(SUCCEEDED(pThis->GetDesc(&desc)))
		{
			D3DLOCKED_RECT lockedRect;
			if(SUCCEEDED(pThis->LockRect(&lockedRect, NULL, D3DLOCK_NOSYSLOCK)))
			{
				int size = lockedRect.Pitch * desc.Height;
				memcpy(lockedRect.pBits, pixels, size);
				pThis->UnlockRect();
			}
		}
	}
	surf8.videoMemoryBackupDirty = FALSE;
}

void BackupVideoMemoryOfAllD3D8Surfaces()
{
	// save the backbuffer surface
	IDirect3DSurface8* pBackBuffer;
	if(d3d8BackBufDirty && s_saved_d3d8Device && SUCCEEDED(s_saved_d3d8Device->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&pBackBuffer)))
	{
		IDirect3DSurface8_CustomData& surf8 = surface8data[pBackBuffer];
		surf8.videoMemoryBackupDirty = true;
		surf8.isBackBuffer = true;
		BackupVideoMemory8(pBackBuffer);
		pBackBuffer->Release();
	}

#ifdef SAVESTATE_DX8_TEXTURES
	// save texture-owned surfaces
	// (textures seem to recycle surfaces internally without properly releasing them,
	//  so I have to access each surface through its texture to avoid crashes)
	for(std::map<IDirect3DTexture8*, IDirect3DTexture8_CustomData>::iterator
		iter = texture8data.begin(); iter != texture8data.end(); iter++)
	{
		IDirect3DTexture8_CustomData& tex8 = iter->second;
		if(!tex8.valid)
			continue;
		IDirect3DTexture8* tex = iter->first;
		DWORD levels = tex->GetLevelCount();
		for(DWORD i = 0; i < levels; i++)
		{
			IDirect3DSurface8* surf;
			if(SUCCEEDED(tex->GetSurfaceLevel(i, &surf)))
			{
				if(tex8.dirty || surface8data[surf].videoMemoryBackupDirty)
				{
					BackupVideoMemory8(surf);
					tex8.dirty = false;
				}
				surf->Release();
			}
		}
	}
	
	// save non-texture-owned surfaces
	for(std::map<IDirect3DSurface8*, IDirect3DSurface8_CustomData>::iterator
		iter = surface8data.begin(); iter != surface8data.end(); iter++)
	{
		IDirect3DSurface8_CustomData& surf8 = iter->second;
		if(surf8.videoMemoryBackupDirty && !surf8.ownedByTexture && !surf8.isBackBuffer)
			BackupVideoMemory8(iter->first);
	}
#endif
}

void RestoreVideoMemoryOfAllD3D8Surfaces()
{
	// load the backbuffer surface
	IDirect3DSurface8* pBackBuffer;
	if(s_saved_d3d8Device && SUCCEEDED(s_saved_d3d8Device->GetBackBuffer(0,D3DBACKBUFFER_TYPE_MONO,&pBackBuffer)))
	{
		RestoreVideoMemory8(pBackBuffer);
		pBackBuffer->Release();
	}

#ifdef SAVESTATE_DX8_TEXTURES
	// load texture-owned surfaces
	// (textures seem to recycle surfaces internally without properly releasing them,
	//  so I have to access each surface through its texture to avoid crashes)
	for(std::map<IDirect3DTexture8*, IDirect3DTexture8_CustomData>::iterator
		iter = texture8data.begin(); iter != texture8data.end(); iter++)
	{
		IDirect3DTexture8_CustomData& tex8 = iter->second;
		if(!tex8.valid)
			continue;
		IDirect3DTexture8* tex = iter->first;
		DWORD levels = tex->GetLevelCount();
		for(DWORD i = 0; i < levels; i++)
		{
			IDirect3DSurface8* surf;
			if(SUCCEEDED(tex->GetSurfaceLevel(i, &surf)))
			{
				if(surface8data[surf].videoMemoryPixelBackup)
					RestoreVideoMemory8(surf);
				surf->Release();
			}
		}
	}
	
	// load non-texture-owned surfaces
	for(std::map<IDirect3DSurface8*, IDirect3DSurface8_CustomData>::iterator
		iter = surface8data.begin(); iter != surface8data.end(); iter++)
	{
		IDirect3DSurface8_CustomData& surf8 = iter->second;
		if(surf8.videoMemoryPixelBackup && !surf8.ownedByTexture && !surf8.isBackBuffer)
			RestoreVideoMemory8(iter->first);
	}
#endif
}



bool RedrawScreenD3D8()
{
	if(s_saved_d3d8Device)
	{
		RECT dstRect;
		RECT* pDstRect = s_savedD3D8pDstRect;
		if(pDstRect && !fakeDisplayValid)
		{
			dstRect = *pDstRect;
			pDstRect = &dstRect;
			RECT oldRect = s_savedD3D8ClientRect;
			RECT newRect;
			GetClientRect(s_savedD3D8HWND, &newRect);
			dstRect.left = (dstRect.left - oldRect.left) * (newRect.right - newRect.left) / (oldRect.right - oldRect.left) + newRect.left;
			dstRect.top = (dstRect.top - oldRect.top) * (newRect.bottom - newRect.top) / (oldRect.bottom - oldRect.top) + newRect.top;
			dstRect.right = (dstRect.right - oldRect.left) * (newRect.right - newRect.left) / (oldRect.right - oldRect.left) + newRect.left;
			dstRect.bottom = (dstRect.bottom - oldRect.top) * (newRect.bottom - newRect.top) / (oldRect.bottom - oldRect.top) + newRect.top;
		}

		HRESULT hr;
		if(s_saved_d3d8SwapChain)
			hr = s_saved_d3d8SwapChain->Present(s_savedD3D8pSrcRect,pDstRect,s_savedD3D8HWND,NULL);
		else
			hr = s_saved_d3d8Device->Present(s_savedD3D8pSrcRect,pDstRect,s_savedD3D8HWND,NULL);
		return true;
	}
	return false;
}






struct MyDirect3D8
{
	static BOOL Hook(IDirect3D8* obj)
	{
		BOOL rv = FALSE;
		rv |= VTHOOKFUNC(IDirect3D8, CreateDevice);
		rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IDirect3D8* pThis, REFIID riid, void** ppvObj);
	static HRESULT STDMETHODCALLTYPE MyQueryInterface(IDirect3D8* pThis, REFIID riid, void** ppvObj)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *CreateDevice)(IDirect3D8* pThis, UINT Adapter,D3DDEVTYPE DeviceType,HWND hFocusWindow,DWORD BehaviorFlags,D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DDevice8** ppReturnedDeviceInterface);
	static HRESULT STDMETHODCALLTYPE MyCreateDevice(IDirect3D8* pThis, UINT Adapter,D3DDEVTYPE DeviceType,HWND hFocusWindow,DWORD BehaviorFlags,D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DDevice8** ppReturnedDeviceInterface)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		ProcessPresentationParams8(pPresentationParameters, pThis, NULL);
		HRESULT rv = CreateDevice(pThis, Adapter,DeviceType,hFocusWindow,BehaviorFlags,pPresentationParameters,ppReturnedDeviceInterface);
		if(SUCCEEDED(rv))
			HookCOMInterface(IID_IDirect3DDevice8, (LPVOID*)ppReturnedDeviceInterface);
		if(pPresentationParameters && pPresentationParameters->hDeviceWindow && IsWindow(pPresentationParameters->hDeviceWindow))
			s_savedD3D8DefaultHWND = pPresentationParameters->hDeviceWindow;
		else
			s_savedD3D8DefaultHWND = hFocusWindow;
		return rv;
	}
};

HRESULT (STDMETHODCALLTYPE* MyDirect3D8::QueryInterface)(IDirect3D8* pThis, REFIID riid, void** ppvObj) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3D8::CreateDevice)(IDirect3D8* pThis, UINT Adapter,D3DDEVTYPE DeviceType,HWND hFocusWindow,DWORD BehaviorFlags,D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DDevice8** ppReturnedDeviceInterface) = 0;



HOOKFUNC IDirect3D8* WINAPI MyDirect3DCreate8(UINT SDKVersion)
{
	debuglog(LCF_D3D, __FUNCTION__ " called.\n");
	ThreadLocalStuff& curtls = tls;
	curtls.callerisuntrusted++;
	IDirect3D8* rv = Direct3DCreate8(SDKVersion);
	if(SUCCEEDED(rv))
		HookCOMInterface(IID_IDirect3D8, (void**)&rv);
	curtls.callerisuntrusted--;
	return rv;
}


bool HookCOMInterfaceD3D8(REFIID riid, LPVOID* ppvOut, bool uncheckedFastNew)
{
	if(!ppvOut)
		return true;

	switch(riid.Data1)
	{
		VTHOOKRIID3(IDirect3D8,MyDirect3D8);
		VTHOOKRIID3(IDirect3DDevice8,MyDirect3DDevice8);
		VTHOOKRIID3(IDirect3DSwapChain8,MyDirect3DSwapChain8);
#ifdef SAVESTATE_DX8_TEXTURES
		VTHOOKRIID3MULTI3(IDirect3DSurface8,MyDirect3DSurface8);
		VTHOOKRIID3(IDirect3DTexture8,MyDirect3DTexture8);
#endif
		default: return false;
	}
	return true;
}

void ApplyD3D8Intercepts()
{
	static const InterceptDescriptor intercepts [] = 
	{
		MAKE_INTERCEPT(1, D3D8, Direct3DCreate8),
	};
	ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
}

#else
#pragma message(__FILE__": (skipped compilation)")
#endif

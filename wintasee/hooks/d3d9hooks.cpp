/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(D3D9HOOKS_INCL) //&& !defined(UNITY_BUILD)
#define D3D9HOOKS_INCL

#include "../../external/d3d9.h"
#include "../global.h"
#include "../wintasee.h"
#include "../tls.h"
#include <map>

//#define SAVESTATE_DX9_TEXTURES

void FakeBroadcastDisplayChange(int width, int height, int depth);

DEFINE_LOCAL_GUID(IID_IDirect3D9,0x81BDCBCA,0x64D4,0x426D,0xAE,0x8D,0xAD,0x1,0x47,0xF4,0x27,0x5C);
DEFINE_LOCAL_GUID(IID_IDirect3DDevice9,0xD0223B96,0xBF7A,0x43FD,0x92,0xBD,0xA4,0x3B,0xD,0x82,0xB9,0xEB);
DEFINE_LOCAL_GUID(IID_IDirect3DSwapChain9,0x794950F2,0xADFC,0x458A,0x90,0x5E,0x10,0xA1,0xB,0xB,0x50,0x3B);
DEFINE_LOCAL_GUID(IID_IDirect3DSurface9,0x0CFBAF3A,0x9FF6,0x429A,0x99,0xB3,0xA2,0x79,0x6A,0xF8,0xB8,0x9B);
DEFINE_LOCAL_GUID(IID_IDirect3DTexture9,0x85C31227,0x3DE5,0x4F00,0x9B,0x3A,0xF1,0x1A,0xC3,0x8C,0x18,0xB5);

static IDirect3DDevice9* pBackBufCopyOwner = NULL;
static IDirect3DSurface9* pBackBufCopy = NULL;
static IDirect3DDevice9* s_saved_d3d9Device = NULL;
static IDirect3DSwapChain9* s_saved_d3d9SwapChain = NULL;
static RECT s_savedD3D9SrcRect = {};
static RECT s_savedD3D9DstRect = {};
static LPRECT s_savedD3D9pSrcRect = NULL;
static LPRECT s_savedD3D9pDstRect = NULL;
static HWND s_savedD3D9HWND = NULL;
static HWND s_savedD3D9DefaultHWND = NULL;
static RECT s_savedD3D9ClientRect = {};

std::map<IDirect3DSwapChain9*,IDirect3DDevice9*> d3d9SwapChainToDeviceMap;

static bool d3d9BackBufActive = true;
static bool d3d9BackBufDirty = true;

// I feel like there must be some smarter way of storing this custom data than with maps,
// but wrapping the interfaces breaks some internal code in d3d that assumes the interface can be cast to a specific internal type,
// and without wrapping the interface I'm not sure how else I could add my own data to the class.

struct IDirect3DSurface9_CustomData
{
	void* videoMemoryPixelBackup;
	bool videoMemoryBackupDirty;
	bool ownedByTexture;
	bool isBackBuffer;
};
static std::map<IDirect3DSurface9*, IDirect3DSurface9_CustomData> surface9data;

struct IDirect3DTexture9_CustomData
{
	bool valid;
	bool dirty;
};
static std::map<IDirect3DTexture9*, IDirect3DTexture9_CustomData> texture9data;


static void ProcessPresentationParams9(D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3D9* d3d, IDirect3DDevice9* d3dDevice)
{
	if(pPresentationParameters) // presentparams
	{
		pPresentationParameters->Flags |= D3DPRESENTFLAG_LOCKABLE_BACKBUFFER; // back buffer must be lockable, to allow for AVI recording
		if(tasflags.forceWindowed && !pPresentationParameters->Windowed)
		{
			pPresentationParameters->Windowed = TRUE;
			pPresentationParameters->FullScreen_RefreshRateInHz = 0;
			
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


struct MyDirect3DDevice9
{
	static BOOL Hook(IDirect3DDevice9* obj)
	{
		BOOL rv = FALSE;
		rv |= VTHOOKFUNC(IDirect3DDevice9, Present);
		rv |= VTHOOKFUNC(IDirect3DDevice9, Reset);
		rv |= VTHOOKFUNC(IDirect3DDevice9, GetSwapChain);
		rv |= VTHOOKFUNC(IDirect3DDevice9, CreateAdditionalSwapChain);
		rv |= VTHOOKFUNC(IDirect3DDevice9, SetRenderTarget);
		rv |= VTHOOKFUNC(IDirect3DDevice9, GetRenderTarget);

		//rv |= VTHOOKFUNC(IDirect3DDevice9, CopyRects);
		//rv |= VTHOOKFUNC(IDirect3DDevice9, UpdateTexture);
		rv |= VTHOOKFUNC(IDirect3DDevice9, Clear);
		rv |= VTHOOKFUNC(IDirect3DDevice9, DrawPrimitive);
		rv |= VTHOOKFUNC(IDirect3DDevice9, DrawIndexedPrimitive);
		rv |= VTHOOKFUNC(IDirect3DDevice9, DrawPrimitiveUP);
		rv |= VTHOOKFUNC(IDirect3DDevice9, DrawIndexedPrimitiveUP);
		//rv |= VTHOOKFUNC(IDirect3DDevice9, DrawRectPatch);
		//rv |= VTHOOKFUNC(IDirect3DDevice9, DrawTriPatch);
		rv |= VTHOOKFUNC(IDirect3DDevice9, Release);
		rv |= VTHOOKFUNC(IDirect3DDevice9, CreateTexture);
		rv |= VTHOOKFUNC(IDirect3DDevice9, CreateRenderTarget);
//		rv |= VTHOOKFUNC(IDirect3DDevice9, CreateImageSurface);
	
	

		rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
		return rv;
	}

	static ULONG(STDMETHODCALLTYPE *Release)(IDirect3DDevice9* pThis);
	static ULONG STDMETHODCALLTYPE MyRelease(IDirect3DDevice9* pThis)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
		ULONG rv = Release(pThis);
		if(rv == 0)
		{
			if(pBackBufCopyOwner == pThis) { pBackBufCopyOwner = NULL; }
			if(s_saved_d3d9Device == pThis) { s_saved_d3d9Device = NULL; }
		}
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IDirect3DDevice9* pThis, REFIID riid, void** ppvObj);
	static HRESULT STDMETHODCALLTYPE MyQueryInterface(IDirect3DDevice9* pThis, REFIID riid, void** ppvObj)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *Reset)(IDirect3DDevice9* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters);
	static HRESULT STDMETHODCALLTYPE MyReset(IDirect3DDevice9* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		ProcessPresentationParams9(pPresentationParameters, NULL, pThis);
		d3d9BackBufActive = true;
		d3d9BackBufDirty = true;
		HRESULT rv = Reset(pThis, pPresentationParameters);
		return rv;
	}

	static void PresentFrameBoundary(IDirect3DDevice9* pThis)
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
			IDirect3DSurface9* pBackBuffer = NULL;
#if 0 // slow
			Lock(pThis, desc, pBackBuffer);
			FrameBoundary(&desc, CAPTUREINFO_TYPE_DDSD);
			pBackBuffer->UnlockRect();
#else // MUCH faster.
	#ifdef _DEBUG
			DWORD time1 = timeGetTime();
	#endif
			pThis->GetRenderTarget(0, &pBackBuffer);
			if(pBackBufCopyOwner != pThis /*&& pBackBufCopy*/)
			{
				//pBackBufCopy->Release();
				pBackBufCopy = NULL;
			}

			IDirect3DSurface9* pSurface = pBackBufCopy;
			if(!pSurface)
			{
				D3DSURFACE_DESC d3ddesc;
				pBackBuffer->GetDesc(&d3ddesc);
				if(SUCCEEDED(pThis->CreateOffscreenPlainSurface(d3ddesc.Width, d3ddesc.Height, d3ddesc.Format, D3DPOOL_SYSTEMMEM, &pBackBufCopy, NULL)))
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
				if(FAILED(pThis->GetRenderTargetData(pBackBuffer,pSurface)))
					pSurface = pBackBuffer;
			if(desc.ddpfPixelFormat.dwRGBBitCount == 8)
				pThis->GetPaletteEntries(0, &activePalette[0]);
			Lock(pThis, desc, pSurface, false);
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

	static HRESULT(STDMETHODCALLTYPE *Present)(IDirect3DDevice9* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion);
	static HRESULT STDMETHODCALLTYPE MyPresent(IDirect3DDevice9* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");

		HRESULT rv;
		if(ShouldSkipDrawing(d3d9BackBufDirty, !d3d9BackBufDirty))
			rv = D3D_OK;
		else
			rv = Present(pThis, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);


		if((d3d9BackBufActive || d3d9BackBufDirty) && !redrawingScreen)
		{
			s_saved_d3d9SwapChain = NULL;
			s_saved_d3d9Device = pThis;
			s_savedD3D9pSrcRect = pSourceRect ? &s_savedD3D9SrcRect : NULL;
			s_savedD3D9pDstRect = pDestRect ? &s_savedD3D9DstRect : NULL;
			if(pSourceRect) s_savedD3D9SrcRect = *pSourceRect;
			if(pDestRect) s_savedD3D9DstRect = *pDestRect;
			s_savedD3D9HWND = hDestWindowOverride ? hDestWindowOverride : s_savedD3D9DefaultHWND;
			GetClientRect(s_savedD3D9HWND, &s_savedD3D9ClientRect);

			PresentFrameBoundary(pThis);
			d3d9BackBufDirty = false;

		}

		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *GetSwapChain)(IDirect3DDevice9* pThis, UINT iSwapChain,IDirect3DSwapChain9** pSwapChain);
	static HRESULT STDMETHODCALLTYPE MyGetSwapChain(IDirect3DDevice9* pThis, UINT iSwapChain,IDirect3DSwapChain9** pSwapChain)
	{
		d3ddebugprintf(__FUNCTION__ "(%d) called.\n", iSwapChain);
		HRESULT rv = GetSwapChain(pThis, iSwapChain, pSwapChain);
		if(SUCCEEDED(rv))
			HookCOMInterface(IID_IDirect3DSwapChain9, (LPVOID*)pSwapChain);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *CreateAdditionalSwapChain)(IDirect3DDevice9* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain9** pSwapChain);
	static HRESULT STDMETHODCALLTYPE MyCreateAdditionalSwapChain(IDirect3DDevice9* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain9** pSwapChain)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		ProcessPresentationParams9(pPresentationParameters, NULL, pThis);
		HRESULT rv = CreateAdditionalSwapChain(pThis, pPresentationParameters, pSwapChain);
		if(SUCCEEDED(rv))
			HookCOMInterface(IID_IDirect3DSwapChain9, (LPVOID*)pSwapChain);
		return rv;
	}

	// DrawPrimitive
	//{
	//	if(ShouldSkipDrawing(false, true))
	//		return D3D_OK;
	//}

	static HRESULT(STDMETHODCALLTYPE *SetRenderTarget)(IDirect3DDevice9* pThis, DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget);
	static HRESULT STDMETHODCALLTYPE MySetRenderTarget(IDirect3DDevice9* pThis, DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget)
	{
		d3ddebugprintf(__FUNCTION__ "(%d) called.\n", RenderTargetIndex);
		HRESULT rv = SetRenderTarget(pThis, RenderTargetIndex, pRenderTarget);
		IDirect3DSurface9* pBackBuffer;
		if(SUCCEEDED(pThis->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&pBackBuffer)))
			if(pBackBuffer->Release() != 0)
				d3d9BackBufActive = (pRenderTarget == pBackBuffer);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *GetRenderTarget)(IDirect3DDevice9* pThis, DWORD RenderTargetIndex, IDirect3DSurface9** ppRenderTarget);
	static HRESULT STDMETHODCALLTYPE MyGetRenderTarget(IDirect3DDevice9* pThis, DWORD RenderTargetIndex, IDirect3DSurface9** ppRenderTarget)
	{
		HRESULT rv = GetRenderTarget(pThis, RenderTargetIndex, ppRenderTarget);
		d3ddebugprintf(__FUNCTION__ "(%d) called, returned 0x%X\n", RenderTargetIndex, (SUCCEEDED(rv) && ppRenderTarget) ? *ppRenderTarget : NULL);
		return rv;
	}

	static void Lock(IDirect3DDevice9* pThis, DDSURFACEDESC& desc, IDirect3DSurface9*& pBackBuffer, bool getBackBuffer=true)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		if(getBackBuffer)
			pThis->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&pBackBuffer);

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

		if(getBackBuffer)
			pBackBuffer->Release();
	}




    static HRESULT(STDMETHODCALLTYPE *Clear)(IDirect3DDevice9* pThis, DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil);
    static HRESULT STDMETHODCALLTYPE MyClear(IDirect3DDevice9* pThis, DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = Clear(pThis, Count,pRects,Flags,Color,Z,Stencil);
		if(!d3d9BackBufDirty && d3d9BackBufActive)
			d3d9BackBufDirty = true;
		return rv;
	}

    static HRESULT(STDMETHODCALLTYPE *DrawPrimitive)(IDirect3DDevice9* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount);
    static HRESULT STDMETHODCALLTYPE MyDrawPrimitive(IDirect3DDevice9* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = DrawPrimitive(pThis, PrimitiveType,StartVertex,PrimitiveCount);
		if(!d3d9BackBufDirty && d3d9BackBufActive)
			d3d9BackBufDirty = true;
		return rv;
	}

    static HRESULT(STDMETHODCALLTYPE *DrawIndexedPrimitive)(IDirect3DDevice9* pThis, D3DPRIMITIVETYPE primitiveType,INT baseVertexIndex,UINT minIndex,UINT NumVertices,UINT startIndex,UINT primCount);
    static HRESULT STDMETHODCALLTYPE MyDrawIndexedPrimitive(IDirect3DDevice9* pThis, D3DPRIMITIVETYPE primitiveType,INT baseVertexIndex,UINT minIndex,UINT NumVertices,UINT startIndex,UINT primCount)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = DrawIndexedPrimitive(pThis, primitiveType,baseVertexIndex,minIndex,NumVertices,startIndex,primCount);
		if(!d3d9BackBufDirty && d3d9BackBufActive)
			d3d9BackBufDirty = true;
		return rv;
	}

    static HRESULT(STDMETHODCALLTYPE *DrawPrimitiveUP)(IDirect3DDevice9* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride);
    static HRESULT STDMETHODCALLTYPE MyDrawPrimitiveUP(IDirect3DDevice9* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = DrawPrimitiveUP(pThis, PrimitiveType,PrimitiveCount,pVertexStreamZeroData,VertexStreamZeroStride);
		if(!d3d9BackBufDirty && d3d9BackBufActive)
			d3d9BackBufDirty = true;
		return rv;
	}

    static HRESULT(STDMETHODCALLTYPE *DrawIndexedPrimitiveUP)(IDirect3DDevice9* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertexIndices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride);
    static HRESULT STDMETHODCALLTYPE MyDrawIndexedPrimitiveUP(IDirect3DDevice9* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertexIndices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = DrawIndexedPrimitiveUP(pThis, PrimitiveType,MinVertexIndex,NumVertexIndices,PrimitiveCount,pIndexData,IndexDataFormat,pVertexStreamZeroData,VertexStreamZeroStride);
		if(!d3d9BackBufDirty && d3d9BackBufActive)
			d3d9BackBufDirty = true;
		return rv;
	}




	static HRESULT(STDMETHODCALLTYPE *CreateTexture)(IDirect3DDevice9* pThis, UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture9** ppTexture,HANDLE* pSharedHandle);
	static HRESULT STDMETHODCALLTYPE MyCreateTexture(IDirect3DDevice9* pThis, UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture9** ppTexture,HANDLE* pSharedHandle)
	{
		d3ddebugprintf(__FUNCTION__ "(%dx%d) called.\n", Width, Height);
		HRESULT rv = CreateTexture(pThis,Width,Height,Levels,Usage,Format,Pool,ppTexture,pSharedHandle);
		if(SUCCEEDED(rv))
		{
			HookCOMInterface(IID_IDirect3DTexture9, reinterpret_cast<LPVOID*>(ppTexture));

			IDirect3DTexture9* pTexture = *ppTexture;
			int numLevels = pTexture->GetLevelCount();
			for(int i = 0; i < numLevels; i++)
			{
				IDirect3DSurface9* pSurface = NULL;
				if(SUCCEEDED(pTexture->GetSurfaceLevel(i, &pSurface)))
				{
					HookCOMInterface(IID_IDirect3DSurface9, reinterpret_cast<LPVOID*>(&pSurface));
					IDirect3DSurface9_CustomData& surf9 = surface9data[pSurface];
					surf9.ownedByTexture = true;
					pSurface->Release();
				}
			}
		}
		return rv;
	}
	static HRESULT(STDMETHODCALLTYPE *CreateRenderTarget)(IDirect3DDevice9* pThis, UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle);
	static HRESULT STDMETHODCALLTYPE MyCreateRenderTarget(IDirect3DDevice9* pThis, UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle)
	{
		d3ddebugprintf(__FUNCTION__ "(%dx%d) called.\n", Width, Height);
		Lockable = TRUE;
		HRESULT rv = CreateRenderTarget(pThis,Width,Height,Format,MultiSample,MultisampleQuality,Lockable,ppSurface,pSharedHandle);
		if(SUCCEEDED(rv))
		{
			HookCOMInterface(IID_IDirect3DSurface9, reinterpret_cast<LPVOID*>(ppSurface));
			IDirect3DSurface9_CustomData& surf9 = surface9data[*ppSurface];
			surf9.ownedByTexture = false;
		}
		return rv;
	}
	//static HRESULT(STDMETHODCALLTYPE *CreateImageSurface)(IDirect3DDevice9* pThis, UINT Width,UINT Height,D3DFORMAT Format,IDirect3DSurface9** ppSurface);
	//static HRESULT STDMETHODCALLTYPE MyCreateImageSurface(IDirect3DDevice9* pThis, UINT Width,UINT Height,D3DFORMAT Format,IDirect3DSurface9** ppSurface)
	//{
	//	d3ddebugprintf(__FUNCTION__ " called.\n");
	//	HRESULT rv = CreateImageSurface(pThis,Width,Height,Format,ppSurface);
	//	return rv;
	//}



};

HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice9::QueryInterface)(IDirect3DDevice9* pThis, REFIID riid, void** ppvObj) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice9::Present)(IDirect3DDevice9* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice9::Reset)(IDirect3DDevice9* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice9::GetSwapChain)(IDirect3DDevice9* pThis, UINT iSwapChain,IDirect3DSwapChain9** pSwapChain) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice9::CreateAdditionalSwapChain)(IDirect3DDevice9* pThis, D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DSwapChain9** pSwapChain) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice9::SetRenderTarget)(IDirect3DDevice9* pThis, DWORD RenderTargetIndex,IDirect3DSurface9* pRenderTarget) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice9::GetRenderTarget)(IDirect3DDevice9* pThis, DWORD RenderTargetIndex,IDirect3DSurface9** ppRenderTarget) = 0;
//HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice9::CopyRects)(IDirect3DDevice9* pThis, IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRectsArray,UINT cRects,IDirect3DSurface9* pDestinationSurface,CONST POINT* pDestPointsArray) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice9::DrawIndexedPrimitive)(IDirect3DDevice9* pThis, D3DPRIMITIVETYPE primitiveType,INT baseVertexIndex,UINT minIndex,UINT NumVertices,UINT startIndex,UINT primCount) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice9::Clear)(IDirect3DDevice9* pThis, DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice9::DrawPrimitive)(IDirect3DDevice9* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice9::DrawPrimitiveUP)(IDirect3DDevice9* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice9::DrawIndexedPrimitiveUP)(IDirect3DDevice9* pThis, D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertexIndices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride) = 0;
ULONG (STDMETHODCALLTYPE* MyDirect3DDevice9::Release)(IDirect3DDevice9* pThis) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice9::CreateTexture)(IDirect3DDevice9* pThis, UINT Width,UINT Height,UINT Levels,DWORD Usage,D3DFORMAT Format,D3DPOOL Pool,IDirect3DTexture9** ppTexture,HANDLE* pSharedHandle) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice9::CreateRenderTarget)(IDirect3DDevice9* pThis, UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Lockable,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle) = 0;



struct MyDirect3DSwapChain9
{
	static BOOL Hook(IDirect3DSwapChain9* obj)
	{
		BOOL rv = FALSE;
		rv |= VTHOOKFUNC(IDirect3DSwapChain9, Present);
		rv |= VTHOOKFUNC(IDirect3DSwapChain9, Release);
		rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
		return rv;
	}

	static ULONG(STDMETHODCALLTYPE *Release)(IDirect3DSwapChain9* pThis);
	static ULONG STDMETHODCALLTYPE MyRelease(IDirect3DSwapChain9* pThis)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
		ULONG rv = Release(pThis);
		if(rv == 0)
		{
			if(s_saved_d3d9SwapChain == pThis) { s_saved_d3d9SwapChain = NULL; }
		}
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IDirect3DSwapChain9* pThis, REFIID riid, void** ppvObj);
	static HRESULT STDMETHODCALLTYPE MyQueryInterface(IDirect3DSwapChain9* pThis, REFIID riid, void** ppvObj)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *Present)(IDirect3DSwapChain9* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion,DWORD dwFlags);
	static HRESULT STDMETHODCALLTYPE MyPresent(IDirect3DSwapChain9* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion,DWORD dwFlags)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");

		HRESULT rv;
		if(ShouldSkipDrawing(true, false))
			rv = D3D_OK;
		else
			rv = Present(pThis, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, dwFlags);

		IDirect3DDevice9* pDevice;
		if(SUCCEEDED(pThis->GetDevice(&pDevice)) && !redrawingScreen)
		{
			s_saved_d3d9SwapChain = pThis;
			s_saved_d3d9Device = pDevice;
			s_savedD3D9pSrcRect = pSourceRect ? &s_savedD3D9SrcRect : NULL;
			s_savedD3D9pDstRect = pDestRect ? &s_savedD3D9DstRect : NULL;
			if(pSourceRect) s_savedD3D9SrcRect = *pSourceRect;
			if(pDestRect) s_savedD3D9DstRect = *pDestRect;
			s_savedD3D9HWND = hDestWindowOverride ? hDestWindowOverride : s_savedD3D9DefaultHWND;
			GetClientRect(s_savedD3D9HWND, &s_savedD3D9ClientRect);

			MyDirect3DDevice9::PresentFrameBoundary(pDevice);
			d3d9BackBufDirty = false;
			pDevice->Release();
		}
		return rv;
	}
};

HRESULT (STDMETHODCALLTYPE* MyDirect3DSwapChain9::QueryInterface)(IDirect3DSwapChain9* pThis, REFIID riid, void** ppvObj) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3DSwapChain9::Present)(IDirect3DSwapChain9* pThis, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion,DWORD dwFlags) = 0;
ULONG (STDMETHODCALLTYPE* MyDirect3DSwapChain9::Release)(IDirect3DSwapChain9* pThis) = 0;



#ifdef SAVESTATE_DX9_TEXTURES

// surfaces are hooked in order to backup their video memory in savestates
template<int IMPL_ID>
struct MyDirect3DSurface9
{
	static void* sm_vtable;
	static BOOL Hook(IDirect3DSurface9* obj)
	{
		void* vtable = (void*)*(size_t*)obj;
		if(sm_vtable && sm_vtable != vtable)
			return 0; // try next IMPL_ID

		VTHOOKFUNC(IDirect3DSurface9, Release);
		VTHOOKFUNC(IDirect3DSurface9, UnlockRect);
		VTHOOKFUNC(IDirect3DSurface9, QueryInterface);
		VTHOOKFUNC(IDirect3DSurface9, FreePrivateData);
		VTHOOKFUNC(IDirect3DSurface9, SetPrivateData);

		if(sm_vtable == vtable)
			return -1; // already hooked

		sm_vtable = vtable;
		return 1; // hooked new
	}

	static ULONG (STDMETHODCALLTYPE *Release)(IDirect3DSurface9* pThis);
	static ULONG STDMETHODCALLTYPE MyRelease(IDirect3DSurface9* pThis)
	{
		ULONG rv = Release(pThis);
		d3ddebugprintf(__FUNCTION__ "(0x%X) called, refcount -> %d.\n", pThis, rv);
		// note: pThis is invalid to dereference now, if rv==0
		if(!rv)
		{
			IDirect3DSurface9_CustomData& surf9 = surface9data[pThis];
			if(!surf9.isBackBuffer) // back buffer's ref count goes to 0 every frame? not sure why
			{
				surf9.videoMemoryBackupDirty = FALSE;

				void*& pixels = surf9.videoMemoryPixelBackup;
				free(pixels);
				pixels = NULL;
			}
		}
		return rv;
	}


    static HRESULT(STDMETHODCALLTYPE *SetPrivateData)(IDirect3DSurface9* pThis, REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags);
    static HRESULT STDMETHODCALLTYPE MySetPrivateData(IDirect3DSurface9* pThis, REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags)
	{
		HRESULT hr = SetPrivateData(pThis,refguid,pData,SizeOfData,Flags);

		IDirect3DSurface9_CustomData& surf9 = surface9data[pThis];
		surf9.videoMemoryBackupDirty = TRUE;

		return hr;
	}

    static HRESULT(STDMETHODCALLTYPE *FreePrivateData)(IDirect3DSurface9* pThis, REFGUID refguid);
    static HRESULT STDMETHODCALLTYPE MyFreePrivateData(IDirect3DSurface9* pThis, REFGUID refguid)
	{
		HRESULT hr = FreePrivateData(pThis,refguid);

		IDirect3DSurface9_CustomData& surf9 = surface9data[pThis];
		surf9.videoMemoryBackupDirty = FALSE;
		void*& pixels = surf9.videoMemoryPixelBackup;
		free(pixels);
		pixels = NULL;

		return hr;
	}


	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IDirect3DSurface9* pThis, REFIID riid, void** ppvObj);
	static HRESULT STDMETHODCALLTYPE MyQueryInterface(IDirect3DSurface9* pThis, REFIID riid, void** ppvObj)
	{
		d3ddebugprintf(__FUNCTION__ " called (0x%X).\n", riid.Data1);
		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *UnlockRect)(IDirect3DSurface9* pThis);
	static HRESULT STDMETHODCALLTYPE MyUnlockRect(IDirect3DSurface9* pThis)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
		HRESULT rv = UnlockRect(pThis);
		IDirect3DSurface9_CustomData& surf9 = surface9data[pThis];
		surf9.videoMemoryBackupDirty = TRUE;
		return rv;
	}
};


#define HRESULT long // some versions of visual studio (including 2008) don't understand "typedef long HRESULT;" (the compiler claims HRESULT is an undefined type even after it's typedef'd to long)

#define DEF(x,IMPL_ID) \
	ULONG (STDMETHODCALLTYPE* MyDirect3DSurface9<IMPL_ID>::Release)(x* pThis) = 0; \
	HRESULT(STDMETHODCALLTYPE* MyDirect3DSurface9<IMPL_ID>::QueryInterface)(x* pThis, REFIID riid, void** ppvObj) = 0; \
    HRESULT(STDMETHODCALLTYPE* MyDirect3DSurface9<IMPL_ID>::UnlockRect)(x* pThis) = 0; \
    HRESULT(STDMETHODCALLTYPE* MyDirect3DSurface9<IMPL_ID>::SetPrivateData)(x* pThis, REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags) = 0; \
    HRESULT(STDMETHODCALLTYPE* MyDirect3DSurface9<IMPL_ID>::FreePrivateData)(x* pThis, REFGUID refguid) = 0; \
	void* MyDirect3DSurface9<IMPL_ID>::sm_vtable = 0;

	DEF(IDirect3DSurface9, 0)
	DEF(IDirect3DSurface9, 1)
	DEF(IDirect3DSurface9, 2)
#undef DEF

#undef HRESULT


struct MyDirect3DTexture9
{
	static BOOL Hook(IDirect3DTexture9* obj)
	{
		BOOL rv = FALSE;
		rv |= VTHOOKFUNC(IDirect3DTexture9, Release);
		rv |= VTHOOKFUNC(IDirect3DTexture9, UnlockRect);
		rv |= VTHOOKFUNC(IDirect3DTexture9, QueryInterface);
		rv |= VTHOOKFUNC(IDirect3DTexture9, FreePrivateData);
		rv |= VTHOOKFUNC(IDirect3DTexture9, SetPrivateData);

		IDirect3DTexture9_CustomData& tex9 = texture9data[obj];
		tex9.valid = true;

		return rv;
	}

	static ULONG (STDMETHODCALLTYPE *Release)(IDirect3DTexture9* pThis);
	static ULONG STDMETHODCALLTYPE MyRelease(IDirect3DTexture9* pThis)
	{
		ULONG rv = Release(pThis);
		d3ddebugprintf(__FUNCTION__ "(0x%X) called, refcount -> %d.\n", pThis, rv);
		// note: pThis is invalid to dereference now, if rv==0
		if(!rv)
		{
			IDirect3DTexture9_CustomData& tex9 = texture9data[pThis];
			tex9.valid = false;
		}
		return rv;
	}


    static HRESULT(STDMETHODCALLTYPE *SetPrivateData)(IDirect3DTexture9* pThis, REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags);
    static HRESULT STDMETHODCALLTYPE MySetPrivateData(IDirect3DTexture9* pThis, REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags)
	{
		HRESULT hr = SetPrivateData(pThis,refguid,pData,SizeOfData,Flags);
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);

		IDirect3DTexture9_CustomData& tex9 = texture9data[pThis];
		tex9.dirty = true;

		return hr;
	}

    static HRESULT(STDMETHODCALLTYPE *FreePrivateData)(IDirect3DTexture9* pThis, REFGUID refguid);
    static HRESULT STDMETHODCALLTYPE MyFreePrivateData(IDirect3DTexture9* pThis, REFGUID refguid)
	{
		HRESULT hr = FreePrivateData(pThis,refguid);
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);

		IDirect3DTexture9_CustomData& tex9 = texture9data[pThis];
		tex9.dirty = false;
		tex9.valid = false;

		return hr;
	}


	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IDirect3DTexture9* pThis, REFIID riid, void** ppvObj);
	static HRESULT STDMETHODCALLTYPE MyQueryInterface(IDirect3DTexture9* pThis, REFIID riid, void** ppvObj)
	{
		d3ddebugprintf(__FUNCTION__ " called (0x%X).\n", riid.Data1);
		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *UnlockRect)(IDirect3DTexture9* pThis, UINT Level);
	static HRESULT STDMETHODCALLTYPE MyUnlockRect(IDirect3DTexture9* pThis, UINT Level)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
		HRESULT rv = UnlockRect(pThis, Level);
		IDirect3DTexture9_CustomData& tex9 = texture9data[pThis];
		tex9.dirty = true;
		return rv;
	}
};

#define HRESULT long // some versions of visual studio (including 2008) don't understand "typedef long HRESULT;" (the compiler claims HRESULT is an undefined type even after it's typedef'd to long)

#define DEF(x) \
	ULONG (STDMETHODCALLTYPE* MyDirect3DTexture9::Release)(x* pThis) = 0; \
	HRESULT(STDMETHODCALLTYPE* MyDirect3DTexture9::QueryInterface)(x* pThis, REFIID riid, void** ppvObj) = 0; \
    HRESULT(STDMETHODCALLTYPE* MyDirect3DTexture9::UnlockRect)(x* pThis, UINT) = 0; \
    HRESULT(STDMETHODCALLTYPE* MyDirect3DTexture9::SetPrivateData)(x* pThis, REFGUID refguid,CONST void* pData,DWORD SizeOfData,DWORD Flags) = 0; \
    HRESULT(STDMETHODCALLTYPE* MyDirect3DTexture9::FreePrivateData)(x* pThis, REFGUID refguid) = 0;

	DEF(IDirect3DTexture9)
#undef DEF

#undef HRESULT


#endif // SAVESTATE_DX9_TEXTURES



static void BackupVideoMemory9(IDirect3DSurface9* pThis)
{
	IDirect3DSurface9_CustomData& surf9 = surface9data[pThis];
	//if(!surf9.videoMemoryBackupDirty)
	//	return;
	D3DSURFACE_DESC desc = {};
	if(SUCCEEDED(pThis->GetDesc(&desc)))
	{
		D3DLOCKED_RECT lockedRect;
		if(SUCCEEDED(pThis->LockRect(&lockedRect, NULL, D3DLOCK_NO_DIRTY_UPDATE|D3DLOCK_READONLY|D3DLOCK_NOSYSLOCK)))
		{
			int size = lockedRect.Pitch * desc.Height;
			void*& pixels = surf9.videoMemoryPixelBackup;
			pixels = realloc(pixels, size);
			memcpy(pixels, lockedRect.pBits, size);
			pThis->UnlockRect();
		}
	}
	surf9.videoMemoryBackupDirty = FALSE;
}

static void RestoreVideoMemory9(IDirect3DSurface9* pThis)
{
	IDirect3DSurface9_CustomData& surf9 = surface9data[pThis];
	if(surf9.videoMemoryBackupDirty)
		return;
	void*& pixels = surf9.videoMemoryPixelBackup;
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
	surf9.videoMemoryBackupDirty = FALSE;
}

void BackupVideoMemoryOfAllD3D9Surfaces()
{
	// save the backbuffer surface
	IDirect3DSurface9* pBackBuffer;
	if(d3d9BackBufDirty && s_saved_d3d9Device)
	{
		int swapCount = s_saved_d3d9Device->GetNumberOfSwapChains();
		for(int i = 0; i < swapCount; i++)
		{
			if(SUCCEEDED(s_saved_d3d9Device->GetBackBuffer(i,0,D3DBACKBUFFER_TYPE_MONO,&pBackBuffer)))
			{
				IDirect3DSurface9_CustomData& surf9 = surface9data[pBackBuffer];
				surf9.videoMemoryBackupDirty = true;
				surf9.isBackBuffer = true;
				BackupVideoMemory9(pBackBuffer);
				pBackBuffer->Release();
			}
		}
	}
#ifdef SAVESTATE_DX9_TEXTURES
	// save texture-owned surfaces
	// (textures seem to recycle surfaces internally without properly releasing them,
	//  so I have to access each surface through its texture to avoid crashes)
	for(std::map<IDirect3DTexture9*, IDirect3DTexture9_CustomData>::iterator
		iter = texture9data.begin(); iter != texture9data.end(); iter++)
	{
		IDirect3DTexture9_CustomData& tex9 = iter->second;
		if(!tex9.valid)
			continue;
		IDirect3DTexture9* tex = iter->first;
		DWORD levels = tex->GetLevelCount();
		for(DWORD i = 0; i < levels; i++)
		{
			IDirect3DSurface9* surf;
			if(SUCCEEDED(tex->GetSurfaceLevel(i, &surf)))
			{
				if(tex9.dirty || surface9data[surf].videoMemoryBackupDirty)
				{
					BackupVideoMemory9(surf);
					tex9.dirty = false;
				}
				surf->Release();
			}
		}
	}
	
	// save non-texture-owned surfaces
	for(std::map<IDirect3DSurface9*, IDirect3DSurface9_CustomData>::iterator
		iter = surface9data.begin(); iter != surface9data.end(); iter++)
	{
		IDirect3DSurface9_CustomData& surf9 = iter->second;
		if(surf9.videoMemoryBackupDirty && !surf9.ownedByTexture && !surf9.isBackBuffer)
			BackupVideoMemory9(iter->first);
	}
#endif
}

void RestoreVideoMemoryOfAllD3D9Surfaces()
{
	// load the backbuffer surface
	IDirect3DSurface9* pBackBuffer;
	if(s_saved_d3d9Device)
	{
		int swapCount = s_saved_d3d9Device->GetNumberOfSwapChains();
		for(int i = 0; i < swapCount; i++)
		{
			if(SUCCEEDED(s_saved_d3d9Device->GetBackBuffer(i,0,D3DBACKBUFFER_TYPE_MONO,&pBackBuffer)))
			{
				RestoreVideoMemory9(pBackBuffer);
				pBackBuffer->Release();
			}
		}
	}
#ifdef SAVESTATE_DX9_TEXTURES
	// load texture-owned surfaces
	// (textures seem to recycle surfaces internally without properly releasing them,
	//  so I have to access each surface through its texture to avoid crashes)
	for(std::map<IDirect3DTexture9*, IDirect3DTexture9_CustomData>::iterator
		iter = texture9data.begin(); iter != texture9data.end(); iter++)
	{
		IDirect3DTexture9_CustomData& tex9 = iter->second;
		if(!tex9.valid)
			continue;
		IDirect3DTexture9* tex = iter->first;
		DWORD levels = tex->GetLevelCount();
		for(DWORD i = 0; i < levels; i++)
		{
			IDirect3DSurface9* surf;
			if(SUCCEEDED(tex->GetSurfaceLevel(i, &surf)))
			{
				if(surface9data[surf].videoMemoryPixelBackup)
					RestoreVideoMemory9(surf);
				surf->Release();
			}
		}
	}
	
	// load non-texture-owned surfaces
	for(std::map<IDirect3DSurface9*, IDirect3DSurface9_CustomData>::iterator
		iter = surface9data.begin(); iter != surface9data.end(); iter++)
	{
		IDirect3DSurface9_CustomData& surf9 = iter->second;
		if(surf9.videoMemoryPixelBackup && !surf9.ownedByTexture && !surf9.isBackBuffer)
			RestoreVideoMemory9(iter->first);
	}
#endif
}



bool RedrawScreenD3D9()
{
	if(s_saved_d3d9Device)
	{
		RECT dstRect;
		RECT* pDstRect = s_savedD3D9pDstRect;
		if(pDstRect && !fakeDisplayValid)
		{
			dstRect = *pDstRect;
			pDstRect = &dstRect;
			RECT oldRect = s_savedD3D9ClientRect;
			RECT newRect;
			GetClientRect(s_savedD3D9HWND, &newRect);
			dstRect.left = (dstRect.left - oldRect.left) * (newRect.right - newRect.left) / (oldRect.right - oldRect.left) + newRect.left;
			dstRect.top = (dstRect.top - oldRect.top) * (newRect.bottom - newRect.top) / (oldRect.bottom - oldRect.top) + newRect.top;
			dstRect.right = (dstRect.right - oldRect.left) * (newRect.right - newRect.left) / (oldRect.right - oldRect.left) + newRect.left;
			dstRect.bottom = (dstRect.bottom - oldRect.top) * (newRect.bottom - newRect.top) / (oldRect.bottom - oldRect.top) + newRect.top;
		}

		HRESULT hr;
		if(s_saved_d3d9SwapChain)
			hr = s_saved_d3d9SwapChain->Present(s_savedD3D9pSrcRect,pDstRect,s_savedD3D9HWND,NULL,0);
		else
			hr = s_saved_d3d9Device->Present(s_savedD3D9pSrcRect,pDstRect,s_savedD3D9HWND,NULL);
		return true;
	}
	return false;
}






struct MyDirect3D9
{
	static BOOL Hook(IDirect3D9* obj)
	{
		BOOL rv = FALSE;
		rv |= VTHOOKFUNC(IDirect3D9, CreateDevice);
		rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IDirect3D9* pThis, REFIID riid, void** ppvObj);
	static HRESULT STDMETHODCALLTYPE MyQueryInterface(IDirect3D9* pThis, REFIID riid, void** ppvObj)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *CreateDevice)(IDirect3D9* pThis, UINT Adapter,D3DDEVTYPE DeviceType,HWND hFocusWindow,DWORD BehaviorFlags,D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DDevice9** ppReturnedDeviceInterface);
	static HRESULT STDMETHODCALLTYPE MyCreateDevice(IDirect3D9* pThis, UINT Adapter,D3DDEVTYPE DeviceType,HWND hFocusWindow,DWORD BehaviorFlags,D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DDevice9** ppReturnedDeviceInterface)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
		ProcessPresentationParams9(pPresentationParameters, pThis, NULL);
		HRESULT rv = CreateDevice(pThis, Adapter,DeviceType,hFocusWindow,BehaviorFlags,pPresentationParameters,ppReturnedDeviceInterface);
		if(SUCCEEDED(rv))
			HookCOMInterface(IID_IDirect3DDevice9, (LPVOID*)ppReturnedDeviceInterface);
		if(pPresentationParameters && pPresentationParameters->hDeviceWindow && IsWindow(pPresentationParameters->hDeviceWindow))
			s_savedD3D9DefaultHWND = pPresentationParameters->hDeviceWindow;
		else
			s_savedD3D9DefaultHWND = hFocusWindow;
		return rv;
	}
};

HRESULT (STDMETHODCALLTYPE* MyDirect3D9::QueryInterface)(IDirect3D9* pThis, REFIID riid, void** ppvObj) = 0;
HRESULT (STDMETHODCALLTYPE* MyDirect3D9::CreateDevice)(IDirect3D9* pThis, UINT Adapter,D3DDEVTYPE DeviceType,HWND hFocusWindow,DWORD BehaviorFlags,D3DPRESENT_PARAMETERS* pPresentationParameters,IDirect3DDevice9** ppReturnedDeviceInterface) = 0;


HOOKFUNC IDirect3D9* WINAPI MyDirect3DCreate9(UINT SDKVersion)
{
	debuglog(LCF_D3D, __FUNCTION__ " called.\n");
	ThreadLocalStuff& curtls = tls;
	curtls.callerisuntrusted++;
	IDirect3D9* rv = Direct3DCreate9(SDKVersion);
	if(SUCCEEDED(rv))
		HookCOMInterface(IID_IDirect3D9, (void**)&rv);
	curtls.callerisuntrusted--;
	return rv;
}


bool HookCOMInterfaceD3D9(REFIID riid, LPVOID* ppvOut, bool uncheckedFastNew)
{
	if(!ppvOut)
		return true;

	switch(riid.Data1)
	{
		VTHOOKRIID3(IDirect3D9,MyDirect3D9);
		VTHOOKRIID3(IDirect3DDevice9,MyDirect3DDevice9);
		VTHOOKRIID3(IDirect3DSwapChain9,MyDirect3DSwapChain9);
#ifdef SAVESTATE_DX9_TEXTURES
		VTHOOKRIID3MULTI3(IDirect3DSurface9,MyDirect3DSurface9);
		VTHOOKRIID3(IDirect3DTexture9,MyDirect3DTexture9);
#endif
		default: return false;
	}
	return true;
}

void ApplyD3D9Intercepts()
{
	static const InterceptDescriptor intercepts [] = 
	{
		MAKE_INTERCEPT(1, D3D9, Direct3DCreate9),
	};
	ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
}

#else
#pragma message(__FILE__": (skipped compilation)")
#endif

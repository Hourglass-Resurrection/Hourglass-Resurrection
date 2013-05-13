/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(D3DHOOKS_INCL) //&& !defined(UNITY_BUILD)
#define D3DHOOKS_INCL

#include "../../external/d3d.h"
#include "../global.h"
#include "../wintasee.h"
#include "../tls.h"
#include <map>

DEFINE_LOCAL_GUID(IID_IDirect3D, 0x3BBA0080,0x2421,0x11CF,0xA3,0x1A,0x00,0xAA,0x00,0xB9,0x33,0x56); //version  < 0x0500
DEFINE_LOCAL_GUID(IID_IDirect3D2,0x6aae1ec1,0x662a,0x11d0,0x88,0x9d,0x00,0xaa,0x00,0xbb,0xb7,0x6a); //version >= 0x0500
DEFINE_LOCAL_GUID(IID_IDirect3D3,0xbb223240,0xe72b,0x11d0,0xa9,0xb4,0x00,0xaa,0x00,0xc0,0x99,0x3e); //version >= 0x0600
DEFINE_LOCAL_GUID(IID_IDirect3D7,0xf5049e77,0x4861,0x11d2,0xa4,0x07,0x00,0xa0,0xc9,0x06,0x29,0xa8); //version >= 0x0700
//DEFINE_LOCAL_GUID(IID_IDirect3D8,0x1dd9e8da,0x1c77,0x4d40,0xb0,0xcf,0x98,0xfe,0xfd,0xff,0x95,0x12); //version >= 0x0800

template<typename IDirect3DN> struct IDirect3DTraits {};
template<> struct IDirect3DTraits<IDirect3D>   { typedef IDirect3DDevice  DIRECT3DDEVICEN; typedef LPDIRECT3DDEVICE  LPDIRECT3DDEVICEN; typedef LPDIRECTDRAWSURFACE  LPDIRECTDRAWSURFACEN; typedef IDirectDrawSurface  DIRECTDRAWSURFACEN; enum{NUMBER=1}; };
template<> struct IDirect3DTraits<IDirect3D2>  { typedef IDirect3DDevice2 DIRECT3DDEVICEN; typedef LPDIRECT3DDEVICE2 LPDIRECT3DDEVICEN; typedef LPDIRECTDRAWSURFACE  LPDIRECTDRAWSURFACEN; typedef IDirectDrawSurface  DIRECTDRAWSURFACEN; enum{NUMBER=2}; };
template<> struct IDirect3DTraits<IDirect3D3>  { typedef IDirect3DDevice3 DIRECT3DDEVICEN; typedef LPDIRECT3DDEVICE3 LPDIRECT3DDEVICEN; typedef LPDIRECTDRAWSURFACE4 LPDIRECTDRAWSURFACEN; typedef IDirectDrawSurface4 DIRECTDRAWSURFACEN; enum{NUMBER=3}; };
template<> struct IDirect3DTraits<IDirect3D7>  { typedef IDirect3DDevice7 DIRECT3DDEVICEN; typedef LPDIRECT3DDEVICE7 LPDIRECT3DDEVICEN; typedef LPDIRECTDRAWSURFACE7 LPDIRECTDRAWSURFACEN; typedef IDirectDrawSurface7 DIRECTDRAWSURFACEN; enum{NUMBER=7}; };


template <typename IDirect3DN>
class MyDirect3D
{
public:
	typedef typename IDirect3DTraits<IDirect3DN>::LPDIRECT3DDEVICEN LPDIRECT3DDEVICEN;
	typedef typename IDirect3DTraits<IDirect3DN>::DIRECT3DDEVICEN IDirect3DDeviceN;
	typedef typename IDirect3DTraits<IDirect3DN>::LPDIRECTDRAWSURFACEN LPDIRECTDRAWSURFACEN;
	typedef typename IDirect3DTraits<IDirect3DN>::DIRECTDRAWSURFACEN DIRECTDRAWSURFACEN;

	static const GUID deviceGuid;
	static const GUID curGuid;

	static BOOL Hook(IDirect3DN* obj)
	{
		//cmdprintf("SHORTTRACE: 3,50");
		BOOL rv = FALSE;
		rv |= VTHOOKFUNC(IDirect3DN, CreateDevice);
		rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IDirect3DN* pThis, REFIID riid, void** ppvObj);
	static HRESULT STDMETHODCALLTYPE MyQueryInterface(IDirect3DN* pThis, REFIID riid, void** ppvObj)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X, 0x%X (0x%X?), 0x%X) called.\n", pThis, riid.Data1, MyDirect3D<IDirect3DN>::curGuid.Data1, ppvObj);
		//if(!ppvObj) { return E_POINTER; }
		//if(riid == IID_IUnknown) { *ppvObj = (IUnknown*)pThis; pThis->AddRef(); return S_OK; }
		//if(IDirect3DTraits<IDirect3DN>::NUMBER == 1 && riid == IID_IDirect3D
		//|| IDirect3DTraits<IDirect3DN>::NUMBER == 2 && riid == IID_IDirect3D2
		//|| IDirect3DTraits<IDirect3DN>::NUMBER == 3 && riid == IID_IDirect3D3
		//|| IDirect3DTraits<IDirect3DN>::NUMBER == 7 && riid == IID_IDirect3D7) { *ppvObj = pThis; pThis->AddRef(); return S_OK; } // ninjah
		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *CreateDevice)(IDirect3DN* pThis, REFCLSID rguid, LPDIRECTDRAWSURFACEN surf, LPDIRECT3DDEVICEN* device);
	static HRESULT STDMETHODCALLTYPE MyCreateDevice(IDirect3DN* pThis, REFCLSID rguid, LPDIRECTDRAWSURFACEN surf, LPDIRECT3DDEVICEN* device)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X, 0x%X?) called.\n", rguid.Data1, MyDirect3D<IDirect3DN>::deviceGuid);
		//HRESULT hr = CreateDevice(pThis, IID_IDirect3DRampDevice, surf, device); // this forces to use a software device
//		FixSurface(surf);
		HRESULT hr = CreateDevice(pThis, rguid, surf, device);
		if(SUCCEEDED(hr))
			HookCOMInterface(MyDirect3D<IDirect3DN>::deviceGuid, (LPVOID*)device);
		else ddrawdebugprintf("CreateDevice failed with hr = 0x%X.\n", hr);
		//cmdprintf("DEBUGPAUSE: B");
		//return -1;
		return hr;
	}

	static HRESULT(STDMETHODCALLTYPE *CreateDevice3)(IDirect3DN* pThis, REFCLSID rguid, LPDIRECTDRAWSURFACEN surf, LPDIRECT3DDEVICEN* device, LPUNKNOWN stupidPointlessArgument);
	static HRESULT STDMETHODCALLTYPE MyCreateDevice3(IDirect3DN* pThis, REFCLSID rguid, LPDIRECTDRAWSURFACEN surf, LPDIRECT3DDEVICEN* device, LPUNKNOWN stupidPointlessArgument)
	{
		d3ddebugprintf(__FUNCTION__ " called.\n");
//		FixSurface(surf);
		HRESULT hr = CreateDevice3(pThis, rguid, surf, device, stupidPointlessArgument);
		if(SUCCEEDED(hr))
			HookCOMInterface(MyDirect3D<IDirect3DN>::deviceGuid, (LPVOID*)device);
		return hr;
	}

};

template<> BOOL MyDirect3D<IDirect3D>::Hook(IDirect3D* obj)
{
	BOOL rv = FALSE;
	//rv |= VTHOOKFUNC(IDirect3D, Initialize);
	rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
	return rv;
}
template<> BOOL MyDirect3D<IDirect3D3>::Hook(IDirect3D3* obj)
{
	BOOL rv = FALSE;
	rv |= VTHOOKFUNC2(IDirect3D3, CreateDevice3, CreateDevice);
	rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
	return rv;
}

template<> const GUID MyDirect3D<IDirect3D>::deviceGuid = IID_IDirect3DDevice;
template<> const GUID MyDirect3D<IDirect3D2>::deviceGuid = IID_IDirect3DDevice2;
template<> const GUID MyDirect3D<IDirect3D3>::deviceGuid = IID_IDirect3DDevice3;
template<> const GUID MyDirect3D<IDirect3D7>::deviceGuid = IID_IDirect3DDevice7;
template<> const GUID MyDirect3D<IDirect3D>::curGuid = IID_IDirect3D;
template<> const GUID MyDirect3D<IDirect3D2>::curGuid = IID_IDirect3D2;
template<> const GUID MyDirect3D<IDirect3D3>::curGuid = IID_IDirect3D3;
template<> const GUID MyDirect3D<IDirect3D7>::curGuid = IID_IDirect3D7;



// stupid C++, have to define statics outside the class,
// and different compiler versions have different bugs with it.
// this (macro-based) way of defining them was the only one that worked on all the compilers I tried

#define HRESULT long // some versions of visual studio (including 2008) don't understand "typedef long HRESULT;" (the compiler claims HRESULT is an undefined type even after it's typedef'd to long)

#define DEF(x) template<> HRESULT (STDMETHODCALLTYPE* MyDirect3D<x>::QueryInterface)(x* pThis, REFIID riid, void** ppvObj) = 0; \
               template<> HRESULT (STDMETHODCALLTYPE* MyDirect3D<x>::CreateDevice)(x* pThis, REFCLSID rguid, LPDIRECTDRAWSURFACEN surf, LPDIRECT3DDEVICEN* device) = 0; \
               template<> HRESULT (STDMETHODCALLTYPE* MyDirect3D<x>::CreateDevice3)(x* pThis, REFCLSID rguid, LPDIRECTDRAWSURFACEN surf, LPDIRECT3DDEVICEN* device, LPUNKNOWN stupidPointlessArgument) = 0;

	DEF(IDirect3D)
	DEF(IDirect3D2)
	DEF(IDirect3D3)
	DEF(IDirect3D7)
#undef DEF

#undef HRESULT



DEFINE_LOCAL_GUID(IID_IDirect3DDevice, 0x64108800,0x957d,0X11d0,0x89,0xab,0x00,0xa0,0xc9,0x05,0x41,0x29);
DEFINE_LOCAL_GUID(IID_IDirect3DDevice2,0x93281501,0x8cf8,0x11d0,0x89,0xab,0x00,0xa0,0xc9,0x05,0x41,0x29);
DEFINE_LOCAL_GUID(IID_IDirect3DDevice3,0xb0ab3b60,0x33d7,0x11d1,0xa9,0x81,0x00,0xc0,0x4f,0xd7,0xb1,0x74);
DEFINE_LOCAL_GUID(IID_IDirect3DDevice7,0xf5049e79,0x4861,0x11d2,0xa4,0x07,0x00,0xa0,0xc9,0x06,0x29,0xa8);

DEFINE_LOCAL_GUID(IID_IDirect3DRampDevice,  0xF2086B20,0x259F,0x11CF,0xA3,0x1A,0x00,0xAA,0x00,0xB9,0x33,0x56);
DEFINE_LOCAL_GUID(IID_IDirect3DRGBDevice,   0xA4665C60,0x2673,0x11CF,0xA3,0x1A,0x00,0xAA,0x00,0xB9,0x33,0x56);
DEFINE_LOCAL_GUID(IID_IDirect3DHALDevice,   0x84E63dE0,0x46AA,0x11CF,0x81,0x6F,0x00,0x00,0xC0,0x20,0x15,0x6E);
DEFINE_LOCAL_GUID(IID_IDirect3DMMXDevice,   0x881949a1,0xd6f3,0x11d0,0x89,0xab,0x00,0xa0,0xc9,0x05,0x41,0x29);
DEFINE_LOCAL_GUID(IID_IDirect3DRefDevice,   0x50936643,0x13e9,0x11d1,0x89,0xaa,0x00,0xa0,0xc9,0x05,0x41,0x29);
DEFINE_LOCAL_GUID(IID_IDirect3DNullDevice,  0x8767df22,0xbacc,0x11d1,0x89,0x69,0x00,0xa0,0xc9,0x06,0x29,0xa8);
DEFINE_LOCAL_GUID(IID_IDirect3DTnLHalDevice,0xf5049e78,0x4861,0x11d2,0xa4,0x07,0x00,0xa0,0xc9,0x06,0x29,0xa8);


template<typename IDirect3DDeviceN> struct IDirect3DDeviceTraits {};
template<> struct IDirect3DDeviceTraits<IDirect3DDevice>  { typedef LPDIRECT3DVERTEXBUFFER  LPDIRECT3DVERTEXBUFFERN; };
template<> struct IDirect3DDeviceTraits<IDirect3DDevice2> { typedef LPDIRECT3DVERTEXBUFFER  LPDIRECT3DVERTEXBUFFERN; };
template<> struct IDirect3DDeviceTraits<IDirect3DDevice3> { typedef LPDIRECT3DVERTEXBUFFER  LPDIRECT3DVERTEXBUFFERN; };
template<> struct IDirect3DDeviceTraits<IDirect3DDevice7> { typedef LPDIRECT3DVERTEXBUFFER7 LPDIRECT3DVERTEXBUFFERN; };


template <typename IDirect3DDeviceN>
struct MyDirect3DDevice
{
	typedef typename IDirect3DDeviceTraits<IDirect3DDeviceN>::LPDIRECT3DVERTEXBUFFERN LPDIRECT3DVERTEXBUFFERN;

	static BOOL Hook(IDirect3DDeviceN* obj)
	{
		BOOL rv = FALSE;
		rv |= VTHOOKFUNC(IDirect3DDeviceN, Release);
		rv |= VTHOOKFUNC(IDirect3DDeviceN, BeginScene);
		rv |= VTHOOKFUNC(IDirect3DDeviceN, EndScene);
		rv |= VTHOOKFUNC(IDirect3DDeviceN, DrawPrimitive);
		rv |= VTHOOKFUNC(IDirect3DDeviceN, DrawIndexedPrimitive);
		rv |= VTHOOKFUNC(IDirect3DDeviceN, DrawPrimitiveStrided);
		rv |= VTHOOKFUNC(IDirect3DDeviceN, DrawIndexedPrimitiveStrided);
		rv |= VTHOOKFUNC(IDirect3DDeviceN, DrawPrimitiveVB);
		rv |= VTHOOKFUNC(IDirect3DDeviceN, DrawIndexedPrimitiveVB);
		rv |= VTHOOKFUNC(IDirect3DDeviceN, Clear);
		rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
		return rv;
	}

	static ULONG (STDMETHODCALLTYPE *Release)(IDirect3DDeviceN* pThis);
	static ULONG STDMETHODCALLTYPE MyRelease(IDirect3DDeviceN* pThis)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
		ULONG rv = Release(pThis);
		return rv;
	}

	static HRESULT(STDMETHODCALLTYPE *QueryInterface)(IDirect3DDeviceN* pThis, REFIID riid, void** ppvObj);
	static HRESULT STDMETHODCALLTYPE MyQueryInterface(IDirect3DDeviceN* pThis, REFIID riid, void** ppvObj)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
		HRESULT rv = QueryInterface(pThis, riid, ppvObj);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppvObj);
		return rv;
	}

	static HRESULT (STDMETHODCALLTYPE *DrawPrimitive)(IDirect3DDeviceN* pThis, D3DPRIMITIVETYPE a, DWORD b, LPVOID c, DWORD d, DWORD e);
	static HRESULT STDMETHODCALLTYPE MyDrawPrimitive(IDirect3DDeviceN* pThis, D3DPRIMITIVETYPE a, DWORD b, LPVOID c, DWORD d, DWORD e)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = DrawPrimitive(pThis, a, b, c, d, e);
		return rv;
	}

	static HRESULT (STDMETHODCALLTYPE *DrawIndexedPrimitive)(IDirect3DDeviceN* pThis, D3DPRIMITIVETYPE a, DWORD b, LPVOID c, DWORD d, LPWORD e, DWORD f, DWORD g);
	static HRESULT STDMETHODCALLTYPE MyDrawIndexedPrimitive(IDirect3DDeviceN* pThis, D3DPRIMITIVETYPE a, DWORD b, LPVOID c, DWORD d, LPWORD e, DWORD f, DWORD g)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = DrawIndexedPrimitive(pThis, a, b, c, d, e, f, g);
		return rv;
	}

	static HRESULT (STDMETHODCALLTYPE *DrawPrimitiveStrided)(IDirect3DDeviceN* pThis, D3DPRIMITIVETYPE a, DWORD b, LPD3DDRAWPRIMITIVESTRIDEDDATA c, DWORD d, DWORD e);
	static HRESULT STDMETHODCALLTYPE MyDrawPrimitiveStrided(IDirect3DDeviceN* pThis, D3DPRIMITIVETYPE a, DWORD b, LPD3DDRAWPRIMITIVESTRIDEDDATA c, DWORD d, DWORD e)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = DrawPrimitiveStrided(pThis, a, b, c, d, e);
		return rv;
	}

	static HRESULT (STDMETHODCALLTYPE *DrawIndexedPrimitiveStrided)(IDirect3DDeviceN* pThis, D3DPRIMITIVETYPE a, DWORD b, LPD3DDRAWPRIMITIVESTRIDEDDATA c, DWORD d, LPWORD e, DWORD f, DWORD g);
	static HRESULT STDMETHODCALLTYPE MyDrawIndexedPrimitiveStrided(IDirect3DDeviceN* pThis, D3DPRIMITIVETYPE a, DWORD b, LPD3DDRAWPRIMITIVESTRIDEDDATA c, DWORD d, LPWORD e, DWORD f, DWORD g)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = DrawIndexedPrimitiveStrided(pThis, a, b, c, d, e, f, g);
		return rv;
	}

	static HRESULT (STDMETHODCALLTYPE *DrawPrimitiveVB)(IDirect3DDeviceN* pThis, D3DPRIMITIVETYPE a, LPDIRECT3DVERTEXBUFFERN b, DWORD c, DWORD d, DWORD e);
	static HRESULT STDMETHODCALLTYPE MyDrawPrimitiveVB(IDirect3DDeviceN* pThis, D3DPRIMITIVETYPE a, LPDIRECT3DVERTEXBUFFERN b, DWORD c, DWORD d, DWORD e)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = DrawPrimitiveVB(pThis, a, b, c, d, e);
		return rv;
	}

	static HRESULT (STDMETHODCALLTYPE *DrawIndexedPrimitiveVB)(IDirect3DDeviceN* pThis, D3DPRIMITIVETYPE a, LPDIRECT3DVERTEXBUFFERN b, DWORD c, DWORD d, LPWORD e, DWORD f, DWORD g);
	static HRESULT STDMETHODCALLTYPE MyDrawIndexedPrimitiveVB(IDirect3DDeviceN* pThis, D3DPRIMITIVETYPE a, LPDIRECT3DVERTEXBUFFERN b, DWORD c, DWORD d, LPWORD e, DWORD f, DWORD g)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = DrawIndexedPrimitiveVB(pThis, a, b, c, d, e, f, g);
		return rv;
	}

	static HRESULT (STDMETHODCALLTYPE *Begin)(IDirect3DDeviceN* pThis, D3DPRIMITIVETYPE a, D3DVERTEXTYPE b, DWORD c);
	static HRESULT STDMETHODCALLTYPE MyBegin(IDirect3DDeviceN* pThis, D3DPRIMITIVETYPE a, D3DVERTEXTYPE b, DWORD c)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = Begin(pThis, a, b, c);
		return rv;
	}

	static HRESULT (STDMETHODCALLTYPE *BeginIndexed)(IDirect3DDeviceN* pThis, D3DPRIMITIVETYPE a, D3DVERTEXTYPE b, LPVOID c, DWORD d, DWORD e);
	static HRESULT STDMETHODCALLTYPE MyBeginIndexed(IDirect3DDeviceN* pThis, D3DPRIMITIVETYPE a, D3DVERTEXTYPE b, LPVOID c, DWORD d, DWORD e)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = BeginIndexed(pThis, a, b, c, d, e);
		return rv;
	}

	static HRESULT (STDMETHODCALLTYPE *Vertex)(IDirect3DDeviceN* pThis, LPVOID a);
	static HRESULT STDMETHODCALLTYPE MyVertex(IDirect3DDeviceN* pThis, LPVOID a)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = Vertex(pThis, a);
		return rv;
	}

	static HRESULT (STDMETHODCALLTYPE *Index)(IDirect3DDeviceN* pThis, WORD a);
	static HRESULT STDMETHODCALLTYPE MyIndex(IDirect3DDeviceN* pThis, WORD a)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = Index(pThis, a);
		return rv;
	}

	static HRESULT (STDMETHODCALLTYPE *End)(IDirect3DDeviceN* pThis, DWORD a);
	static HRESULT STDMETHODCALLTYPE MyEnd(IDirect3DDeviceN* pThis, DWORD a)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = End(pThis, a);
		return rv;
	}

	static HRESULT (STDMETHODCALLTYPE *BeginScene)(IDirect3DDeviceN* pThis);
	static HRESULT STDMETHODCALLTYPE MyBeginScene(IDirect3DDeviceN* pThis)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
		HRESULT rv = BeginScene(pThis);
		return rv;
	}

	static HRESULT (STDMETHODCALLTYPE *EndScene)(IDirect3DDeviceN* pThis);
	static HRESULT STDMETHODCALLTYPE MyEndScene(IDirect3DDeviceN* pThis)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
		HRESULT rv = EndScene(pThis);

		return rv;
	}

	static HRESULT (STDMETHODCALLTYPE *Clear)(IDirect3DDeviceN* pThis, DWORD a,LPD3DRECT b,DWORD c,D3DCOLOR d,D3DVALUE e,DWORD f);
	static HRESULT STDMETHODCALLTYPE MyClear(IDirect3DDeviceN* pThis, DWORD a,LPD3DRECT b,DWORD c,D3DCOLOR d,D3DVALUE e,DWORD f)
	{
		d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
		if(ShouldSkipDrawing(false, true))
			return D3D_OK;
		HRESULT rv = Clear(pThis,a,b,c,d,e,f);
		return rv;
	}
};

// these D3D versions had fewer drawing functions
template<> BOOL MyDirect3DDevice<IDirect3DDevice>::Hook(IDirect3DDevice* obj)
{
	BOOL rv = FALSE;
	rv |= VTHOOKFUNC(IDirect3DDevice, Release);
	rv |= VTHOOKFUNC(IDirect3DDevice, BeginScene);
	rv |= VTHOOKFUNC(IDirect3DDevice, EndScene);
	rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
	return rv;
}
template<> BOOL MyDirect3DDevice<IDirect3DDevice2>::Hook(IDirect3DDevice2* obj)
{
	BOOL rv = FALSE;
	rv |= VTHOOKFUNC(IDirect3DDevice2, Release);
	rv |= VTHOOKFUNC(IDirect3DDevice2, BeginScene);
	rv |= VTHOOKFUNC(IDirect3DDevice2, EndScene);
	rv |= VTHOOKFUNC(IDirect3DDevice2, DrawPrimitive);
	rv |= VTHOOKFUNC(IDirect3DDevice2, DrawIndexedPrimitive);
	rv |= VTHOOKFUNC(IDirect3DDevice2, Begin);
	rv |= VTHOOKFUNC(IDirect3DDevice2, BeginIndexed);
	rv |= VTHOOKFUNC(IDirect3DDevice2, Vertex);
	rv |= VTHOOKFUNC(IDirect3DDevice2, Index);
	rv |= VTHOOKFUNC(IDirect3DDevice2, End);
	rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
	return rv;
}
template<> BOOL MyDirect3DDevice<IDirect3DDevice3>::Hook(IDirect3DDevice3* obj)
{
	BOOL rv = FALSE;
	rv |= VTHOOKFUNC(IDirect3DDevice3, Release);
	rv |= VTHOOKFUNC(IDirect3DDevice3, BeginScene);
	rv |= VTHOOKFUNC(IDirect3DDevice3, EndScene);
	rv |= VTHOOKFUNC(IDirect3DDevice3, DrawPrimitive);
	rv |= VTHOOKFUNC(IDirect3DDevice3, DrawIndexedPrimitive);
	rv |= VTHOOKFUNC(IDirect3DDevice3, DrawPrimitiveStrided);
	rv |= VTHOOKFUNC(IDirect3DDevice3, DrawIndexedPrimitiveStrided);
	rv |= VTHOOKFUNC(IDirect3DDevice3, DrawPrimitiveVB);
	rv |= VTHOOKFUNC(IDirect3DDevice3, DrawIndexedPrimitiveVB);
	rv |= VTHOOKFUNC(IDirect3DDevice3, Begin);
	rv |= VTHOOKFUNC(IDirect3DDevice3, BeginIndexed);
	rv |= VTHOOKFUNC(IDirect3DDevice3, Vertex);
	rv |= VTHOOKFUNC(IDirect3DDevice3, Index);
	rv |= VTHOOKFUNC(IDirect3DDevice3, End);
	rv |= HookVTable(obj, 0, (FARPROC)MyQueryInterface, (FARPROC&)QueryInterface, __FUNCTION__": QueryInterface");
	return rv;
}



//HRESULT (STDMETHODCALLTYPE *D3DEndScene)(void* pThis) = 0;
//__declspec(noinline) HRESULT STDMETHODCALLTYPE MyD3DEndScene(void* pThis)
//{
//	//d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", pThis);
//	HRESULT rv = D3DEndScene(pThis);
//	//FrameBoundary(); // disabled, d3dsurface handles it
//	return rv;
//}


// stupid C++, have to define statics outside the class,
// and different compiler versions have different bugs with it.
// this (macro-based) way of defining them was the only one that worked on all the compilers I tried

#define HRESULT long // some versions of visual studio (including 2008) don't understand "typedef long HRESULT;" (the compiler claims HRESULT is an undefined type even after it's typedef'd to long)

#define DEF(x) template<> ULONG (STDMETHODCALLTYPE* MyDirect3DDevice<x>::Release)(x* pThis) = 0; \
               template<> HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice<x>::QueryInterface)(x* pThis, REFIID riid, void** ppvObj) = 0; \
               template<> HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice<x>::BeginScene)(x* pThis) = 0; \
               template<> HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice<x>::EndScene)(x* pThis) = 0; \
               template<> HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice<x>::DrawPrimitive)(x* pThis, D3DPRIMITIVETYPE a, DWORD b, LPVOID c, DWORD d, DWORD e) = 0; \
               template<> HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice<x>::DrawIndexedPrimitive)(x* pThis, D3DPRIMITIVETYPE a, DWORD b, LPVOID c, DWORD d, LPWORD e, DWORD f, DWORD g) = 0; \
               template<> HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice<x>::DrawPrimitiveStrided)(x* pThis, D3DPRIMITIVETYPE a, DWORD b, LPD3DDRAWPRIMITIVESTRIDEDDATA c, DWORD d, DWORD e); \
               template<> HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice<x>::DrawIndexedPrimitiveStrided)(x* pThis, D3DPRIMITIVETYPE a, DWORD b, LPD3DDRAWPRIMITIVESTRIDEDDATA c, DWORD d, LPWORD e, DWORD f, DWORD g); \
               template<> HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice<x>::DrawPrimitiveVB)(x* pThis, D3DPRIMITIVETYPE a, LPDIRECT3DVERTEXBUFFERN b, DWORD c, DWORD d, DWORD e); \
               template<> HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice<x>::DrawIndexedPrimitiveVB)(x* pThis, D3DPRIMITIVETYPE a, LPDIRECT3DVERTEXBUFFERN b, DWORD c, DWORD d, LPWORD e, DWORD f, DWORD g); \
               template<> HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice<x>::Begin)(x* pThis, D3DPRIMITIVETYPE,D3DVERTEXTYPE,DWORD); \
               template<> HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice<x>::BeginIndexed)(x* pThis, D3DPRIMITIVETYPE,D3DVERTEXTYPE,LPVOID,DWORD,DWORD); \
               template<> HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice<x>::Vertex)(x* pThis, LPVOID); \
               template<> HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice<x>::Index)(x* pThis, WORD); \
               template<> HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice<x>::End)(x* pThis, DWORD); \
               template<> HRESULT (STDMETHODCALLTYPE* MyDirect3DDevice<x>::Clear)(x* pThis, DWORD a,LPD3DRECT b,DWORD c,D3DCOLOR d,D3DVALUE e,DWORD f);

	DEF(IDirect3DDevice)
	DEF(IDirect3DDevice2)
	DEF(IDirect3DDevice3)
	DEF(IDirect3DDevice7)
#undef DEF

#undef HRESULT




HOOKFUNC HRESULT WINAPI MyDirect3DCreate(UINT SDKVersion, LPUNKNOWN* lplpd3d, LPUNKNOWN pUnkOuter)
{
	d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", SDKVersion);
	ThreadLocalStuff& curtls = tls;
	curtls.callerisuntrusted++;
	HRESULT rv = Direct3DCreate(SDKVersion, lplpd3d, pUnkOuter);
	if(SUCCEEDED(rv))
	{
		HookCOMInterfaceUnknownVT(IDirect3D, *lplpd3d);
		//HookCOMInterface(IID_IDirect3D, (LPVOID*)lplpd3d);
	}
	curtls.callerisuntrusted--;
	return rv;
}
HOOKFUNC HRESULT WINAPI MyDirect3DCreate7(UINT SDKVersion, LPUNKNOWN* lplpd3d, LPUNKNOWN pUnkOuter)
{
	d3ddebugprintf(__FUNCTION__ "(0x%X) called.\n", SDKVersion);
	ThreadLocalStuff& curtls = tls;
	curtls.callerisuntrusted++;
	HRESULT rv = Direct3DCreate7(SDKVersion, lplpd3d, pUnkOuter);
	if(SUCCEEDED(rv))
	{
		HookCOMInterfaceUnknownVT(IDirect3D7, *lplpd3d);
		//HookCOMInterface(IID_IDirect3D7, (LPVOID*)lplpd3d); // doesn't work in Ninjah
	}
	curtls.callerisuntrusted--;
	return rv;
}
HOOKFUNC HRESULT WINAPI MyDirect3DCreateDevice(GUID FAR *lpGUID, LPUNKNOWN lpd3ddevice, LPDIRECTDRAWSURFACE surf, LPUNKNOWN* lplpd3ddevice, LPUNKNOWN pUnkOuter)
{
	// maybe unnecessary?
	d3ddebugprintf(__FUNCTION__ "(0x%X, 0x%X) called.\n", lpGUID->Data1, lpd3ddevice);
	ThreadLocalStuff& curtls = tls;
	curtls.callerisuntrusted++;
	HRESULT rv = Direct3DCreateDevice(lpGUID, lpd3ddevice, surf, lplpd3ddevice, pUnkOuter);
	if(SUCCEEDED(rv))
		HookCOMInterfaceUnknownVT(IDirect3DDevice, *lplpd3ddevice);
	curtls.callerisuntrusted--;
	return rv;
}
HOOKFUNC HRESULT WINAPI MyDirect3DCreateDevice7(GUID FAR *lpGUID, LPUNKNOWN lpd3ddevice, LPDIRECTDRAWSURFACE surf, LPUNKNOWN* lplpd3ddevice, LPUNKNOWN pUnkOuter)
{
	// maybe unnecessary?
	d3ddebugprintf(__FUNCTION__ "(0x%X, 0x%X) called.\n", lpGUID->Data1, lpd3ddevice);
	ThreadLocalStuff& curtls = tls;
	curtls.callerisuntrusted++;
	HRESULT rv = Direct3DCreateDevice7(lpGUID, lpd3ddevice, surf, lplpd3ddevice, pUnkOuter);
	if(SUCCEEDED(rv))
		HookCOMInterfaceUnknownVT(IDirect3DDevice7, *lplpd3ddevice);
	curtls.callerisuntrusted--;
	return rv;
}

bool HookCOMInterfaceD3D7(REFIID riid, LPVOID* ppvOut, bool uncheckedFastNew)
{
	if(!ppvOut)
		return true;

	switch(riid.Data1)
	{
		VTHOOKRIID(Direct3D,);
		VTHOOKRIID(Direct3D,2);
		VTHOOKRIID(Direct3D,3);
		VTHOOKRIID(Direct3D,7);
		//HOOKRIID(Direct3D,);
		//HOOKRIID(Direct3D,2);
		//HOOKRIID(Direct3D,3);
		//HOOKRIID(Direct3D,7);

		VTHOOKRIID(Direct3DDevice,);
		VTHOOKRIID(Direct3DDevice,2);
		VTHOOKRIID(Direct3DDevice,3);
		VTHOOKRIID(Direct3DDevice,7);

		default: return false;
	}
	return true;
}

void ApplyD3DIntercepts()
{
	static const InterceptDescriptor intercepts [] = 
	{
		MAKE_INTERCEPT(1, D3DIM, Direct3DCreate),
		MAKE_INTERCEPT(1, D3DIM, Direct3DCreateDevice),
		MAKE_INTERCEPT2(1, D3DIM700, Direct3DCreate, Direct3DCreate7),
		MAKE_INTERCEPT2(1, D3DIM700, Direct3DCreateDevice, Direct3DCreateDevice7),
	};
	ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
}

#else
#pragma message(__FILE__": (skipped compilation)")
#endif

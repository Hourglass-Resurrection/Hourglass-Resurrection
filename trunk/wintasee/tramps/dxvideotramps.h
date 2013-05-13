/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef DXVIDEOTRAMPS_H_INCL
#define DXVIDEOTRAMPS_H_INCL

#define DirectDrawCreate TrampDirectDrawCreate
TRAMPFUNC HRESULT WINAPI DirectDrawCreate(GUID FAR *lpGUID, struct IDirectDraw* FAR *lplpDD, IUnknown FAR *pUnkOuter) TRAMPOLINE_DEF
#define DirectDrawCreateEx TrampDirectDrawCreateEx
TRAMPFUNC HRESULT WINAPI DirectDrawCreateEx(GUID FAR * lpGuid, LPVOID  *lplpDD, REFIID  iid,IUnknown FAR *pUnkOuter) TRAMPOLINE_DEF
#define CheckFullscreen TrampCheckFullscreen
TRAMPFUNC HRESULT WINAPI CheckFullscreen() TRAMPOLINE_DEF


#define Direct3DCreate9 TrampDirect3DCreate9
TRAMPFUNC struct IDirect3D9* WINAPI Direct3DCreate9(UINT SDKVersion) TRAMPOLINE_DEF
#define Direct3DCreate8 TrampDirect3DCreate8
TRAMPFUNC struct IDirect3D8* WINAPI Direct3DCreate8(UINT SDKVersion) TRAMPOLINE_DEF
#define Direct3DCreate TrampDirect3DCreate
TRAMPFUNC HRESULT WINAPI Direct3DCreate(UINT SDKVersion, LPUNKNOWN* lplpd3d, LPUNKNOWN pUnkOuter) TRAMPOLINE_DEF
#define Direct3DCreate7 TrampDirect3DCreate7
TRAMPFUNC HRESULT WINAPI Direct3DCreate7(UINT SDKVersion, LPUNKNOWN* lplpd3d, LPUNKNOWN pUnkOuter) TRAMPOLINE_DEF
#define Direct3DCreateDevice TrampDirect3DCreateDevice
TRAMPFUNC HRESULT WINAPI Direct3DCreateDevice(GUID FAR *lpGUID, LPUNKNOWN lpd3ddevice, struct IDirectDrawSurface* surf, LPUNKNOWN* lplpd3ddevice, LPUNKNOWN pUnkOuter) TRAMPOLINE_DEF
#define Direct3DCreateDevice7 TrampDirect3DCreateDevice7
TRAMPFUNC HRESULT WINAPI Direct3DCreateDevice7(GUID FAR *lpGUID, LPUNKNOWN lpd3ddevice, struct IDirectDrawSurface* surf, LPUNKNOWN* lplpd3ddevice, LPUNKNOWN pUnkOuter) TRAMPOLINE_DEF

#endif

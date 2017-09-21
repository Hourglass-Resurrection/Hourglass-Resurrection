/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#include "../intercept.h"

struct IDirect3D8;
struct IDirect3D9;
struct IDirect3D9Ex;

namespace Hooks
{
    extern BOOL fakeDisplayValid;
    extern int fakeDisplayWidth;
    extern int fakeDisplayHeight;
    extern int fakePixelFormatBPP;
    extern int fakeDisplayRefresh;

    HOOK_FUNCTION_DECLARE(HRESULT,
                          WINAPI,
                          DirectDrawCreate,
                          GUID FAR* lpGUID,
                          struct IDirectDraw* FAR* lplpDD,
                          IUnknown FAR* pUnkOuter);
    HOOK_FUNCTION_DECLARE(HRESULT,
                          WINAPI,
                          DirectDrawCreateEx,
                          GUID FAR* lpGuid,
                          LPVOID* lplpDD,
                          REFIID iid,
                          IUnknown FAR* pUnkOuter);
    HOOK_FUNCTION_DECLARE(HRESULT, WINAPI, CheckFullscreen);

    HOOK_FUNCTION_DECLARE(IDirect3D9*, WINAPI, Direct3DCreate9, UINT SDKVersion);
    HOOK_FUNCTION_DECLARE(HRESULT,
                          WINAPI,
                          Direct3DCreate9Ex,
                          UINT SDKVersion,
                          IDirect3D9Ex** ppD3D);
    HOOK_FUNCTION_DECLARE(IDirect3D8*, WINAPI, Direct3DCreate8, UINT SDKVersion);
    HOOK_FUNCTION_DECLARE(HRESULT,
                          WINAPI,
                          Direct3DCreate,
                          UINT SDKVersion,
                          LPUNKNOWN* lplpd3d,
                          LPUNKNOWN pUnkOuter);
    HOOK_FUNCTION_DECLARE(HRESULT,
                          WINAPI,
                          Direct3DCreate7,
                          UINT SDKVersion,
                          LPUNKNOWN* lplpd3d,
                          LPUNKNOWN pUnkOuter);
    HOOK_FUNCTION_DECLARE(HRESULT,
                          WINAPI,
                          Direct3DCreateDevice,
                          GUID FAR* lpGUID,
                          LPUNKNOWN lpd3ddevice,
                          struct IDirectDrawSurface* surf,
                          LPUNKNOWN* lplpd3ddevice,
                          LPUNKNOWN pUnkOuter);
    HOOK_FUNCTION_DECLARE(HRESULT,
                          WINAPI,
                          Direct3DCreateDevice7,
                          GUID FAR* lpGUID,
                          LPUNKNOWN lpd3ddevice,
                          struct IDirectDrawSurface* surf,
                          LPUNKNOWN* lplpd3ddevice,
                          LPUNKNOWN pUnkOuter);

    void BackupVideoMemoryOfAllDDrawSurfaces();
    void BackupVideoMemoryOfAllD3D8Surfaces();
    void BackupVideoMemoryOfAllD3D9Surfaces();
    void RestoreVideoMemoryOfAllDDrawSurfaces();
    void RestoreVideoMemoryOfAllD3D8Surfaces();
    void RestoreVideoMemoryOfAllD3D9Surfaces();
    bool RedrawScreenD3D8();
    bool RedrawScreenD3D9();
    bool RedrawScreenDDraw();

    void ApplyD3D8Intercepts();
    void ApplyD3D9Intercepts();
    void ApplyD3DIntercepts();
    void ApplyDDrawIntercepts();

    bool HookCOMInterfaceD3D7(REFIID riid, LPVOID* ppvOut, bool uncheckedFastNew);
    bool HookCOMInterfaceD3D8(REFIID riid, LPVOID* ppvOut, bool uncheckedFastNew);
    bool HookCOMInterfaceD3D9(REFIID riid, LPVOID* ppvOut, bool uncheckedFastNew);
    bool HookCOMInterfaceDDraw(REFIID riid, LPVOID* ppvOut, bool uncheckedFastNew);
}

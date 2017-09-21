/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#include "../intercept.h"

namespace Hooks
{
    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          StretchBlt,
                          HDC hdcDest,
                          int nXOriginDest,
                          int nYOriginDest,
                          int nWidthDest,
                          int nHeightDest,
                          HDC hdcSrc,
                          int nXOriginSrc,
                          int nYOriginSrc,
                          int nWidthSrc,
                          int nHeightSrc,
                          DWORD dwRop);
    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          BitBlt,
                          HDC a,
                          int b,
                          int c,
                          int d,
                          int e,
                          HDC f,
                          int g,
                          int h,
                          DWORD i);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, SwapBuffers, HDC hdc);
    HOOK_FUNCTION_DECLARE(int,
                          WINAPI,
                          SetDIBitsToDevice,
                          HDC hdc,
                          int xDest,
                          int yDest,
                          DWORD w,
                          DWORD h,
                          int xSrc,
                          int ySrc,
                          UINT StartScan,
                          UINT cLines,
                          CONST VOID* lpvBits,
                          CONST BITMAPINFO* lpbmi,
                          UINT ColorUse);
    HOOK_FUNCTION_DECLARE(int,
                          WINAPI,
                          StretchDIBits,
                          HDC hdc,
                          int xDest,
                          int yDest,
                          int DestWidth,
                          int DestHeight,
                          int xSrc,
                          int ySrc,
                          int SrcWidth,
                          int SrcHeight,
                          CONST VOID* lpBits,
                          CONST BITMAPINFO* lpbmi,
                          UINT iUsage,
                          DWORD rop);

    HOOK_FUNCTION_DECLARE(int,
                          WINAPI,
                          ChoosePixelFormat,
                          HDC hdc,
                          CONST PIXELFORMATDESCRIPTOR* pfd);
    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          SetPixelFormat,
                          HDC hdc,
                          int format,
                          CONST PIXELFORMATDESCRIPTOR* pfd);
    HOOK_FUNCTION_DECLARE(COLORREF, WINAPI, GetPixel, HDC hdc, int xx, int yy);
    HOOK_FUNCTION_DECLARE(int, WINAPI, GetDeviceCaps, HDC hdc, int index);

    HOOK_FUNCTION_DECLARE(HFONT, WINAPI, CreateFontIndirectA, CONST LOGFONTA* lplf);
    HOOK_FUNCTION_DECLARE(HFONT, WINAPI, CreateFontIndirectW, CONST LOGFONTW* lplf);

    // not technically GDI but as video-related win32 functions they're close enough.
    HOOK_FUNCTION_DECLARE(LONG,
                          WINAPI,
                          ChangeDisplaySettingsA,
                          LPDEVMODEA lpDevMode,
                          DWORD dwFlags);
    HOOK_FUNCTION_DECLARE(LONG,
                          WINAPI,
                          ChangeDisplaySettingsW,
                          LPDEVMODEW lpDevMode,
                          DWORD dwFlags);

    bool RedrawScreenGDI();

    void ApplyGDIIntercepts();
}

/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef GDITRAMPS_H_INCL
#define GDITRAMPS_H_INCL

#define StretchBlt TrampStretchBlt
TRAMPFUNC BOOL WINAPI StretchBlt(HDC hdcDest, int nXOriginDest, int nYOriginDest, int nWidthDest, int nHeightDest, HDC hdcSrc, int nXOriginSrc, int nYOriginSrc, int nWidthSrc, int nHeightSrc, DWORD dwRop) TRAMPOLINE_DEF
#define BitBlt TrampBitBlt
TRAMPFUNC BOOL WINAPI BitBlt(HDC a, int b, int c, int d, int e, HDC f, int g, int h, DWORD i) TRAMPOLINE_DEF
#define SwapBuffers TrampSwapBuffers
TRAMPFUNC BOOL SwapBuffers(HDC hdc) TRAMPOLINE_DEF
#define SetDIBitsToDevice TrampSetDIBitsToDevice
TRAMPFUNC int WINAPI SetDIBitsToDevice(HDC hdc, int xDest, int yDest, DWORD w, DWORD h, int xSrc, int ySrc, UINT StartScan, UINT cLines, CONST VOID * lpvBits, CONST BITMAPINFO * lpbmi, UINT ColorUse) TRAMPOLINE_DEF
#define StretchDIBits TrampStretchDIBits
TRAMPFUNC int WINAPI StretchDIBits(HDC hdc, int xDest, int yDest, int DestWidth, int DestHeight, int xSrc, int ySrc, int SrcWidth, int SrcHeight, CONST VOID * lpBits, CONST BITMAPINFO * lpbmi, UINT iUsage, DWORD rop) TRAMPOLINE_DEF

#define ChoosePixelFormat TrampChoosePixelFormat
TRAMPFUNC int WINAPI ChoosePixelFormat(HDC hdc, CONST PIXELFORMATDESCRIPTOR* pfd) TRAMPOLINE_DEF
#define SetPixelFormat TrampSetPixelFormat
TRAMPFUNC BOOL WINAPI SetPixelFormat(HDC hdc, int format, CONST PIXELFORMATDESCRIPTOR * pfd) TRAMPOLINE_DEF
#define GetPixel TrampGetPixel
TRAMPFUNC COLORREF WINAPI GetPixel(HDC hdc, int xx, int yy) TRAMPOLINE_DEF
#define GetDeviceCaps TrampGetDeviceCaps
TRAMPFUNC int WINAPI GetDeviceCaps(HDC hdc, int index) TRAMPOLINE_DEF

#define CreateFontIndirectA TrampCreateFontIndirectA
TRAMPFUNC HFONT WINAPI CreateFontIndirectA(CONST LOGFONTA *lplf) TRAMPOLINE_DEF
#define CreateFontIndirectW TrampCreateFontIndirectW
TRAMPFUNC HFONT WINAPI CreateFontIndirectW(CONST LOGFONTW *lplf) TRAMPOLINE_DEF

// not technically GDI but as video-related win32 functions they're close enough.
#define ChangeDisplaySettingsA TrampChangeDisplaySettingsA
TRAMPFUNC LONG WINAPI ChangeDisplaySettingsA(LPDEVMODEA lpDevMode, DWORD dwFlags) TRAMPOLINE_DEF
#define ChangeDisplaySettingsW TrampChangeDisplaySettingsW
TRAMPFUNC LONG WINAPI ChangeDisplaySettingsW(LPDEVMODEW lpDevMode, DWORD dwFlags) TRAMPOLINE_DEF

#endif

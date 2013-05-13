/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef INPUTTRAMPS_H_INCL
#define INPUTTRAMPS_H_INCL

#define DirectInputCreateA TrampDirectInputCreateA
TRAMPFUNC HRESULT WINAPI DirectInputCreateA(HINSTANCE hinst, DWORD dwVersion, struct LPDIRECTINPUTA *ppDI, LPUNKNOWN punkOuter) TRAMPOLINE_DEF
#define DirectInputCreateW TrampDirectInputCreateW
TRAMPFUNC HRESULT WINAPI DirectInputCreateW(HINSTANCE hinst, DWORD dwVersion, struct LPDIRECTINPUTW *ppDI, LPUNKNOWN punkOuter) TRAMPOLINE_DEF
#define DirectInputCreateEx TrampDirectInputCreateEx
TRAMPFUNC HRESULT WINAPI DirectInputCreateEx(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter) TRAMPOLINE_DEF
#define DirectInput8Create TrampDirectInput8Create
TRAMPFUNC HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter) TRAMPOLINE_DEF

#define GetAsyncKeyState TrampGetAsyncKeyState
TRAMPFUNC SHORT WINAPI GetAsyncKeyState(int vKey) TRAMPOLINE_DEF
#define GetKeyState TrampGetKeyState
TRAMPFUNC SHORT WINAPI GetKeyState(int vKey) TRAMPOLINE_DEF
#define GetKeyboardState TrampGetKeyboardState
TRAMPFUNC BOOL WINAPI GetKeyboardState(PBYTE lpKeyState) TRAMPOLINE_DEF
#define GetLastInputInfo TrampGetLastInputInfo
TRAMPFUNC BOOL WINAPI GetLastInputInfo(PLASTINPUTINFO plii) TRAMPOLINE_DEF

#define GetCursorPos TrampGetCursorPos
TRAMPFUNC BOOL WINAPI GetCursorPos(LPPOINT lpPoint) TRAMPOLINE_DEF
#define GetCursorInfo TrampGetCursorInfo
TRAMPFUNC BOOL WINAPI GetCursorInfo(PCURSORINFO pci) TRAMPOLINE_DEF

#define joySetCapture TrampjoySetCapture
TRAMPFUNC MMRESULT WINAPI joySetCapture(HWND hwnd, UINT uJoyID, UINT uPeriod, BOOL fChanged) TRAMPOLINE_DEF
#define joyReleaseCapture TrampjoyReleaseCapture
TRAMPFUNC MMRESULT WINAPI joyReleaseCapture(UINT uJoyID) TRAMPOLINE_DEF
#define joyGetPosEx TrampjoyGetPosEx
TRAMPFUNC MMRESULT WINAPI joyGetPosEx(UINT uJoyID, LPJOYINFOEX pji) TRAMPOLINE_DEF
#define joyGetPos TrampjoyGetPos
TRAMPFUNC MMRESULT WINAPI joyGetPos(UINT uJoyID, LPJOYINFO pji) TRAMPOLINE_DEF
#define joyGetNumDevs TrampjoyGetNumDevs
TRAMPFUNC UINT WINAPI joyGetNumDevs() TRAMPOLINE_DEF
#define joyGetDevCapsA TrampjoyGetDevCapsA
TRAMPFUNC MMRESULT WINAPI joyGetDevCapsA(UINT_PTR uJoyID, LPJOYCAPSA pjc, UINT cbjc) TRAMPOLINE_DEF
#define joyGetDevCapsW TrampjoyGetDevCapsW
TRAMPFUNC MMRESULT WINAPI joyGetDevCapsW(UINT_PTR uJoyID, LPJOYCAPSW pjc, UINT cbjc) TRAMPOLINE_DEF

#endif

/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#define DirectInputCreateA TrampDirectInputCreateA
TRAMPFUNC HRESULT WINAPI DirectInputCreateA(HINSTANCE hinst, DWORD dwVersion, struct LPDIRECTINPUTA *ppDI, LPUNKNOWN punkOuter);
#define DirectInputCreateW TrampDirectInputCreateW
TRAMPFUNC HRESULT WINAPI DirectInputCreateW(HINSTANCE hinst, DWORD dwVersion, struct LPDIRECTINPUTW *ppDI, LPUNKNOWN punkOuter);
#define DirectInputCreateEx TrampDirectInputCreateEx
TRAMPFUNC HRESULT WINAPI DirectInputCreateEx(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter);
#define DirectInput8Create TrampDirectInput8Create
TRAMPFUNC HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter);

#define GetAsyncKeyState TrampGetAsyncKeyState
TRAMPFUNC SHORT WINAPI GetAsyncKeyState(int vKey);
#define GetKeyState TrampGetKeyState
TRAMPFUNC SHORT WINAPI GetKeyState(int vKey);
#define GetKeyboardState TrampGetKeyboardState
TRAMPFUNC BOOL WINAPI GetKeyboardState(PBYTE lpKeyState);
#define GetLastInputInfo TrampGetLastInputInfo
TRAMPFUNC BOOL WINAPI GetLastInputInfo(PLASTINPUTINFO plii);

#define GetCursorPos TrampGetCursorPos
TRAMPFUNC BOOL WINAPI GetCursorPos(LPPOINT lpPoint);
#define GetCursorInfo TrampGetCursorInfo
TRAMPFUNC BOOL WINAPI GetCursorInfo(PCURSORINFO pci);

#define joySetCapture TrampjoySetCapture
TRAMPFUNC MMRESULT WINAPI joySetCapture(HWND hwnd, UINT uJoyID, UINT uPeriod, BOOL fChanged);
#define joyReleaseCapture TrampjoyReleaseCapture
TRAMPFUNC MMRESULT WINAPI joyReleaseCapture(UINT uJoyID);
#define joyGetPosEx TrampjoyGetPosEx
TRAMPFUNC MMRESULT WINAPI joyGetPosEx(UINT uJoyID, LPJOYINFOEX pji);
#define joyGetPos TrampjoyGetPos
TRAMPFUNC MMRESULT WINAPI joyGetPos(UINT uJoyID, LPJOYINFO pji);
#define joyGetNumDevs TrampjoyGetNumDevs
TRAMPFUNC UINT WINAPI joyGetNumDevs();
#define joyGetDevCapsA TrampjoyGetDevCapsA
TRAMPFUNC MMRESULT WINAPI joyGetDevCapsA(UINT_PTR uJoyID, LPJOYCAPSA pjc, UINT cbjc);
#define joyGetDevCapsW TrampjoyGetDevCapsW
TRAMPFUNC MMRESULT WINAPI joyGetDevCapsW(UINT_PTR uJoyID, LPJOYCAPSW pjc, UINT cbjc);

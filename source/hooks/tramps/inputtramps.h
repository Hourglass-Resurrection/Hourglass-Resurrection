/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#include "../intercept.h"

namespace Hooks
{
    extern int g_midFrameAsyncKeyRequests;

    HOOK_FUNCTION_DECLARE(HRESULT, WINAPI, DirectInputCreateA, HINSTANCE hinst, DWORD dwVersion, struct LPDIRECTINPUTA *ppDI, LPUNKNOWN punkOuter);
    HOOK_FUNCTION_DECLARE(HRESULT, WINAPI, DirectInputCreateW, HINSTANCE hinst, DWORD dwVersion, struct LPDIRECTINPUTW *ppDI, LPUNKNOWN punkOuter);
    HOOK_FUNCTION_DECLARE(HRESULT, WINAPI, DirectInputCreateEx, HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter);
    HOOK_FUNCTION_DECLARE(HRESULT, WINAPI, DirectInput8Create, HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID *ppvOut, LPUNKNOWN punkOuter);

    HOOK_FUNCTION_DECLARE(SHORT, WINAPI, GetAsyncKeyState, int vKey);
    HOOK_FUNCTION_DECLARE(SHORT, WINAPI, GetKeyState, int vKey);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, GetKeyboardState, PBYTE lpKeyState);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, GetLastInputInfo, PLASTINPUTINFO plii);

    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, GetCursorPos, LPPOINT lpPoint);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, GetCursorInfo, PCURSORINFO pci);

    HOOK_FUNCTION_DECLARE(MMRESULT, WINAPI, joySetCapture, HWND hwnd, UINT uJoyID, UINT uPeriod, BOOL fChanged);
    HOOK_FUNCTION_DECLARE(MMRESULT, WINAPI, joyReleaseCapture, UINT uJoyID);
    HOOK_FUNCTION_DECLARE(MMRESULT, WINAPI, joyGetPosEx, UINT uJoyID, LPJOYINFOEX pji);
    HOOK_FUNCTION_DECLARE(MMRESULT, WINAPI, joyGetPos, UINT uJoyID, LPJOYINFO pji);
    HOOK_FUNCTION_DECLARE(UINT, WINAPI, joyGetNumDevs);
    HOOK_FUNCTION_DECLARE(MMRESULT, WINAPI, joyGetDevCapsA, UINT_PTR uJoyID, LPJOYCAPSA pjc, UINT cbjc);
    HOOK_FUNCTION_DECLARE(MMRESULT, WINAPI, joyGetDevCapsW, UINT_PTR uJoyID, LPJOYCAPSW pjc, UINT cbjc);

    void ProcessFrameInput();

    void ApplyInputIntercepts();

    bool HookCOMInterfaceInput(REFIID riid, LPVOID* ppvOut, bool uncheckedFastNew);
    bool HookCOMInterfaceInputEx(REFIID riid, LPVOID* ppvOut, REFGUID parameter, bool uncheckedFastNew);
}

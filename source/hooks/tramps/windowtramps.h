/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#include <map>

#include "../intercept.h"

namespace Hooks
{
    extern std::map<HWND, WNDPROC> hwndToOrigHandler;

    HOOK_DECLARE(HWND, WINAPI, CreateWindowExA, DWORD dwExStyle, LPCSTR lpClassName,
        LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
        HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
    HOOK_DECLARE(HWND, WINAPI, CreateWindowExW, DWORD dwExStyle, LPCWSTR lpClassName,
        LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
        HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
    HOOK_DECLARE(BOOL, WINAPI, DestroyWindow, HWND hWnd);
    HOOK_DECLARE(BOOL, WINAPI, CloseWindow, HWND hWnd);
    HOOK_DECLARE(LONG, WINAPI, SetWindowLongA, HWND hWnd, int nIndex, LONG dwNewLong);
    HOOK_DECLARE(LONG, WINAPI, SetWindowLongW, HWND hWnd, int nIndex, LONG dwNewLong);
    HOOK_DECLARE(LONG, WINAPI, GetWindowLongA, HWND hWnd, int nIndex);
    HOOK_DECLARE(LONG, WINAPI, GetWindowLongW, HWND hWnd, int nIndex);

    //HOOK_DECLARE(BOOL, WINAPI, InvalidateRect, HWND hWnd, CONST RECT *lpRect, BOOL bErase)
    //HOOK_DECLARE(BOOL, WINAPI, UpdateWindow(HWND hWnd);

    HOOK_DECLARE(int, WINAPI, MessageBoxA, HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);
    HOOK_DECLARE(int, WINAPI, MessageBoxW, HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);
    HOOK_DECLARE(int, WINAPI, MessageBoxExA, HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType, WORD wLanguageId);
    HOOK_DECLARE(int, WINAPI, MessageBoxExW, HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType, WORD wLanguageId);
    HOOK_DECLARE(INT_PTR, WINAPI, DialogBoxParamA, HINSTANCE hInstance, LPCSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
    HOOK_DECLARE(INT_PTR, WINAPI, DialogBoxParamW, HINSTANCE hInstance, LPCWSTR lpTemplateName, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
    HOOK_DECLARE(INT_PTR, WINAPI, DialogBoxIndirectParamA, HINSTANCE hInstance, LPCDLGTEMPLATEA hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);
    HOOK_DECLARE(INT_PTR, WINAPI, DialogBoxIndirectParamW, HINSTANCE hInstance, LPCDLGTEMPLATEW hDialogTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam);

    HOOK_DECLARE(BOOL, WINAPI, MoveWindow, HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint);
    HOOK_DECLARE(BOOL, WINAPI, SetWindowPos, HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);
    HOOK_DECLARE(BOOL, WINAPI, ShowWindow, HWND hWnd, int nCmdShow);
    HOOK_DECLARE(BOOL, WINAPI, GetClientRect, HWND hWnd, LPRECT lpRect);
    HOOK_DECLARE(BOOL, WINAPI, GetWindowRect, HWND hWnd, LPRECT lpRect);
    HOOK_DECLARE(BOOL, WINAPI, ClientToScreen, HWND hWnd, LPPOINT lpPoint);
    HOOK_DECLARE(BOOL, WINAPI, ScreenToClient, HWND hWnd, LPPOINT lpPoint);
    HOOK_DECLARE(BOOL, WINAPI, SetWindowTextA, HWND hWnd, LPCSTR lpString);
    HOOK_DECLARE(BOOL, WINAPI, SetWindowTextW, HWND hWnd, LPCWSTR lpString);

    HOOK_DECLARE(HWND, WINAPI, GetActiveWindow);
    HOOK_DECLARE(HWND, WINAPI, GetForegroundWindow);
    HOOK_DECLARE(HWND, WINAPI, GetFocus);

    void ApplyWindowIntercepts();
}

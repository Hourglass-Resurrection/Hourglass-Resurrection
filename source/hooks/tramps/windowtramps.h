/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#define CreateWindowExA TrampCreateWindowExA
TRAMPFUNC HWND WINAPI CreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName,
	LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
	HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
#define CreateWindowExW TrampCreateWindowExW
TRAMPFUNC HWND WINAPI CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName,
	LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
	HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
#define DestroyWindow TrampDestroyWindow
TRAMPFUNC BOOL WINAPI DestroyWindow(HWND hWnd);
#define CloseWindow TrampCloseWindow
TRAMPFUNC BOOL WINAPI CloseWindow(HWND hWnd);
#define SetWindowLongA TrampSetWindowLongA
TRAMPFUNC LONG WINAPI SetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong);
#define SetWindowLongW TrampSetWindowLongW
TRAMPFUNC LONG WINAPI SetWindowLongW(HWND hWnd, int nIndex, LONG dwNewLong);
#define GetWindowLongA TrampGetWindowLongA
TRAMPFUNC LONG WINAPI GetWindowLongA(HWND hWnd, int nIndex);
#define GetWindowLongW TrampGetWindowLongW
TRAMPFUNC LONG WINAPI GetWindowLongW(HWND hWnd, int nIndex);


//#define InvalidateRect TrampInvalidateRect
//TRAMPFUNC BOOL WINAPI InvalidateRect(HWND hWnd,CONST RECT *lpRect,BOOL bErase);
//#define UpdateWindow TrampUpdateWindow
//TRAMPFUNC BOOL WINAPI UpdateWindow(HWND hWnd);

#define MessageBoxA TrampMessageBoxA
TRAMPFUNC int WINAPI MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);
#define MessageBoxW TrampMessageBoxW
TRAMPFUNC int WINAPI MessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);
#define MessageBoxExA TrampMessageBoxExA
TRAMPFUNC int WINAPI MessageBoxExA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType, WORD wLanguageId);
#define MessageBoxExW TrampMessageBoxExW
TRAMPFUNC int WINAPI MessageBoxExW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType, WORD wLanguageId);
#define DialogBoxParamA TrampDialogBoxParamA
TRAMPFUNC INT_PTR WINAPI DialogBoxParamA(HINSTANCE hInstance,LPCSTR lpTemplateName,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam);
#define DialogBoxParamW TrampDialogBoxParamW
TRAMPFUNC INT_PTR WINAPI DialogBoxParamW(HINSTANCE hInstance,LPCWSTR lpTemplateName,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam);
#define DialogBoxIndirectParamA TrampDialogBoxIndirectParamA
TRAMPFUNC INT_PTR WINAPI DialogBoxIndirectParamA(HINSTANCE hInstance,LPCDLGTEMPLATEA hDialogTemplate,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam);
#define DialogBoxIndirectParamW TrampDialogBoxIndirectParamW
TRAMPFUNC INT_PTR WINAPI DialogBoxIndirectParamW(HINSTANCE hInstance,LPCDLGTEMPLATEW hDialogTemplate,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam);

#define MoveWindow TrampMoveWindow
TRAMPFUNC BOOL WINAPI MoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint);
#define SetWindowPos TrampSetWindowPos
TRAMPFUNC BOOL WINAPI SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);
#define ShowWindow TrampShowWindow
TRAMPFUNC BOOL WINAPI ShowWindow(HWND hWnd, int nCmdShow);
#define GetClientRect TrampGetClientRect
TRAMPFUNC BOOL WINAPI GetClientRect(HWND hWnd, LPRECT lpRect);
#define GetWindowRect TrampGetWindowRect
TRAMPFUNC BOOL WINAPI GetWindowRect(HWND hWnd, LPRECT lpRect);
#define ClientToScreen TrampClientToScreen
TRAMPFUNC BOOL WINAPI ClientToScreen(HWND hWnd, LPPOINT lpPoint);
#define ScreenToClient TrampScreenToClient
TRAMPFUNC BOOL WINAPI ScreenToClient(HWND hWnd, LPPOINT lpPoint);
#define SetWindowTextA TrampSetWindowTextA
TRAMPFUNC BOOL WINAPI SetWindowTextA(HWND hWnd, LPCSTR lpString);
#define SetWindowTextW TrampSetWindowTextW
TRAMPFUNC BOOL WINAPI SetWindowTextW(HWND hWnd, LPCWSTR lpString);

#define GetActiveWindow TrampGetActiveWindow
TRAMPFUNC HWND WINAPI GetActiveWindow();
#define GetForegroundWindow TrampGetForegroundWindow
TRAMPFUNC HWND WINAPI GetForegroundWindow();
#define GetFocus TrampGetFocus
TRAMPFUNC HWND WINAPI GetFocus();

/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef WINDOWTRAMPS_H_INCL
#define WINDOWTRAMPS_H_INCL

#define CreateWindowExA TrampCreateWindowExA
TRAMPFUNC HWND WINAPI CreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName,
	LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
	HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) TRAMPOLINE_DEF
#define CreateWindowExW TrampCreateWindowExW
TRAMPFUNC HWND WINAPI CreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName,
	LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight,
	HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) TRAMPOLINE_DEF
#define DestroyWindow TrampDestroyWindow
TRAMPFUNC BOOL WINAPI DestroyWindow(HWND hWnd) TRAMPOLINE_DEF
#define CloseWindow TrampCloseWindow
TRAMPFUNC BOOL WINAPI CloseWindow(HWND hWnd) TRAMPOLINE_DEF
#define SetWindowLongA TrampSetWindowLongA
TRAMPFUNC LONG WINAPI SetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong) TRAMPOLINE_DEF
#define SetWindowLongW TrampSetWindowLongW
TRAMPFUNC LONG WINAPI SetWindowLongW(HWND hWnd, int nIndex, LONG dwNewLong) TRAMPOLINE_DEF
#define GetWindowLongA TrampGetWindowLongA
TRAMPFUNC LONG WINAPI GetWindowLongA(HWND hWnd, int nIndex) TRAMPOLINE_DEF
#define GetWindowLongW TrampGetWindowLongW
TRAMPFUNC LONG WINAPI GetWindowLongW(HWND hWnd, int nIndex) TRAMPOLINE_DEF


//#define InvalidateRect TrampInvalidateRect
//TRAMPFUNC BOOL WINAPI InvalidateRect(HWND hWnd,CONST RECT *lpRect,BOOL bErase) TRAMPOLINE_DEF
//#define UpdateWindow TrampUpdateWindow
//TRAMPFUNC BOOL WINAPI UpdateWindow(HWND hWnd) TRAMPOLINE_DEF

#define MessageBoxA TrampMessageBoxA
TRAMPFUNC int WINAPI MessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) TRAMPOLINE_DEF
#define MessageBoxW TrampMessageBoxW
TRAMPFUNC int WINAPI MessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType) TRAMPOLINE_DEF
#define MessageBoxExA TrampMessageBoxExA
TRAMPFUNC int WINAPI MessageBoxExA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType, WORD wLanguageId) TRAMPOLINE_DEF
#define MessageBoxExW TrampMessageBoxExW
TRAMPFUNC int WINAPI MessageBoxExW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType, WORD wLanguageId) TRAMPOLINE_DEF
#define DialogBoxParamA TrampDialogBoxParamA
TRAMPFUNC INT_PTR WINAPI DialogBoxParamA(HINSTANCE hInstance,LPCSTR lpTemplateName,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam) TRAMPOLINE_DEF
#define DialogBoxParamW TrampDialogBoxParamW
TRAMPFUNC INT_PTR WINAPI DialogBoxParamW(HINSTANCE hInstance,LPCWSTR lpTemplateName,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam) TRAMPOLINE_DEF
#define DialogBoxIndirectParamA TrampDialogBoxIndirectParamA
TRAMPFUNC INT_PTR WINAPI DialogBoxIndirectParamA(HINSTANCE hInstance,LPCDLGTEMPLATEA hDialogTemplate,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam) TRAMPOLINE_DEF
#define DialogBoxIndirectParamW TrampDialogBoxIndirectParamW
TRAMPFUNC INT_PTR WINAPI DialogBoxIndirectParamW(HINSTANCE hInstance,LPCDLGTEMPLATEW hDialogTemplate,HWND hWndParent,DLGPROC lpDialogFunc,LPARAM dwInitParam) TRAMPOLINE_DEF

#define MoveWindow TrampMoveWindow
TRAMPFUNC BOOL WINAPI MoveWindow(HWND hWnd, int X, int Y, int nWidth, int nHeight, BOOL bRepaint) TRAMPOLINE_DEF
#define SetWindowPos TrampSetWindowPos
TRAMPFUNC BOOL WINAPI SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags) TRAMPOLINE_DEF
#define ShowWindow TrampShowWindow
TRAMPFUNC BOOL WINAPI ShowWindow(HWND hWnd, int nCmdShow) TRAMPOLINE_DEF
#define GetClientRect TrampGetClientRect
TRAMPFUNC BOOL WINAPI GetClientRect(HWND hWnd, LPRECT lpRect) TRAMPOLINE_DEF
#define GetWindowRect TrampGetWindowRect
TRAMPFUNC BOOL WINAPI GetWindowRect(HWND hWnd, LPRECT lpRect) TRAMPOLINE_DEF
#define ClientToScreen TrampClientToScreen
TRAMPFUNC BOOL WINAPI ClientToScreen(HWND hWnd, LPPOINT lpPoint) TRAMPOLINE_DEF
#define ScreenToClient TrampScreenToClient
TRAMPFUNC BOOL WINAPI ScreenToClient(HWND hWnd, LPPOINT lpPoint) TRAMPOLINE_DEF
#define SetWindowTextA TrampSetWindowTextA
TRAMPFUNC BOOL WINAPI SetWindowTextA(HWND hWnd, LPCSTR lpString) TRAMPOLINE_DEF
#define SetWindowTextW TrampSetWindowTextW
TRAMPFUNC BOOL WINAPI SetWindowTextW(HWND hWnd, LPCWSTR lpString) TRAMPOLINE_DEF

#define GetActiveWindow TrampGetActiveWindow
TRAMPFUNC HWND WINAPI GetActiveWindow() TRAMPOLINE_DEF
#define GetForegroundWindow TrampGetForegroundWindow
TRAMPFUNC HWND WINAPI GetForegroundWindow() TRAMPOLINE_DEF
#define GetFocus TrampGetFocus
TRAMPFUNC HWND WINAPI GetFocus() TRAMPOLINE_DEF

#endif

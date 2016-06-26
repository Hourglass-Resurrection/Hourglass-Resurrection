/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#define SendNotifyMessageA TrampSendNotifyMessageA
TRAMPFUNC BOOL WINAPI SendNotifyMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
#define SendNotifyMessageW TrampSendNotifyMessageW
TRAMPFUNC BOOL WINAPI SendNotifyMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
#define SendMessageTimeoutA TrampSendMessageTimeoutA
TRAMPFUNC LRESULT WINAPI SendMessageTimeoutA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT fuFlags, UINT uTimeout, PDWORD_PTR lpdwResult);
#define SendMessageTimeoutW TrampSendMessageTimeoutW
TRAMPFUNC LRESULT WINAPI SendMessageTimeoutW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT fuFlags, UINT uTimeout, PDWORD_PTR lpdwResult);
#define SendMessageCallbackA TrampSendMessageCallbackA
TRAMPFUNC LRESULT WINAPI SendMessageCallbackA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, SENDASYNCPROC lpResultCallBack, ULONG_PTR dwData);
#define SendMessageCallbackW TrampSendMessageCallbackW
TRAMPFUNC LRESULT WINAPI SendMessageCallbackW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, SENDASYNCPROC lpResultCallBack, ULONG_PTR dwData);
#define SendMessageW TrampSendMessageW
TRAMPFUNC LRESULT WINAPI SendMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
#define SendMessageA TrampSendMessageA
TRAMPFUNC LRESULT WINAPI SendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
#define PostMessageW TrampPostMessageW
TRAMPFUNC BOOL WINAPI PostMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
#define PostMessageA TrampPostMessageA
TRAMPFUNC BOOL WINAPI PostMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
#define PostThreadMessageA TrampPostThreadMessageA
TRAMPFUNC BOOL WINAPI PostThreadMessageA(DWORD idThread,UINT Msg,WPARAM wParam,LPARAM lParam);
#define PostThreadMessageW TrampPostThreadMessageW
TRAMPFUNC BOOL WINAPI PostThreadMessageW(DWORD idThread,UINT Msg,WPARAM wParam,LPARAM lParam);
#define PostQuitMessage TrampPostQuitMessage
TRAMPFUNC VOID WINAPI PostQuitMessage(int nExitCode);


#define GetMessageA TrampGetMessageA
TRAMPFUNC BOOL WINAPI GetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
#define GetMessageW TrampGetMessageW
TRAMPFUNC BOOL WINAPI GetMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax);
#define PeekMessageA TrampPeekMessageA
TRAMPFUNC BOOL WINAPI PeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
#define PeekMessageW TrampPeekMessageW
TRAMPFUNC BOOL WINAPI PeekMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg);
#define GetQueueStatus TrampGetQueueStatus
TRAMPFUNC DWORD WINAPI GetQueueStatus(UINT flags);
#define GetInputState TrampGetInputState
TRAMPFUNC BOOL WINAPI GetInputState();
#define GetMessagePos TrampGetMessagePos
TRAMPFUNC DWORD WINAPI GetMessagePos(VOID);
#define GetMessageTime TrampGetMessageTime
TRAMPFUNC LONG WINAPI GetMessageTime(VOID);

#define TranslateMessage TrampTranslateMessage
TRAMPFUNC BOOL WINAPI TranslateMessage(CONST MSG *lpMsg);
#define DispatchMessageA TrampDispatchMessageA
TRAMPFUNC LRESULT WINAPI DispatchMessageA(CONST MSG *lpMsg);
#define DispatchMessageW TrampDispatchMessageW
TRAMPFUNC LRESULT WINAPI DispatchMessageW(CONST MSG *lpMsg);

#define RegisterWindowMessageA TrampRegisterWindowMessageA
TRAMPFUNC UINT WINAPI RegisterWindowMessageA(LPCSTR lpString);
#define RegisterWindowMessageW TrampRegisterWindowMessageW
TRAMPFUNC UINT WINAPI RegisterWindowMessageW(LPCWSTR lpString);

#define DefWindowProcA TrampDefWindowProcA
TRAMPFUNC LRESULT WINAPI DefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
#define DefWindowProcW TrampDefWindowProcW
TRAMPFUNC LRESULT WINAPI DefWindowProcW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

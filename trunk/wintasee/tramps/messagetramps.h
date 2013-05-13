/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef MESSAGE_H_INCL
#define MESSAGE_H_INCL

#define SendNotifyMessageA TrampSendNotifyMessageA
TRAMPFUNC BOOL WINAPI SendNotifyMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) TRAMPOLINE_DEF
#define SendNotifyMessageW TrampSendNotifyMessageW
TRAMPFUNC BOOL WINAPI SendNotifyMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) TRAMPOLINE_DEF
#define SendMessageTimeoutA TrampSendMessageTimeoutA
TRAMPFUNC LRESULT WINAPI SendMessageTimeoutA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT fuFlags, UINT uTimeout, PDWORD_PTR lpdwResult) TRAMPOLINE_DEF
#define SendMessageTimeoutW TrampSendMessageTimeoutW
TRAMPFUNC LRESULT WINAPI SendMessageTimeoutW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, UINT fuFlags, UINT uTimeout, PDWORD_PTR lpdwResult) TRAMPOLINE_DEF
#define SendMessageCallbackA TrampSendMessageCallbackA
TRAMPFUNC LRESULT WINAPI SendMessageCallbackA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, SENDASYNCPROC lpResultCallBack, ULONG_PTR dwData) TRAMPOLINE_DEF
#define SendMessageCallbackW TrampSendMessageCallbackW
TRAMPFUNC LRESULT WINAPI SendMessageCallbackW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam, SENDASYNCPROC lpResultCallBack, ULONG_PTR dwData) TRAMPOLINE_DEF
#define SendMessageW TrampSendMessageW
TRAMPFUNC LRESULT WINAPI SendMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) TRAMPOLINE_DEF
#define SendMessageA TrampSendMessageA
TRAMPFUNC LRESULT WINAPI SendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) TRAMPOLINE_DEF
#define PostMessageW TrampPostMessageW
TRAMPFUNC BOOL WINAPI PostMessageW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) TRAMPOLINE_DEF
#define PostMessageA TrampPostMessageA
TRAMPFUNC BOOL WINAPI PostMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) TRAMPOLINE_DEF
#define PostThreadMessageA TrampPostThreadMessageA
TRAMPFUNC BOOL WINAPI PostThreadMessageA(DWORD idThread,UINT Msg,WPARAM wParam,LPARAM lParam) TRAMPOLINE_DEF
#define PostThreadMessageW TrampPostThreadMessageW
TRAMPFUNC BOOL WINAPI PostThreadMessageW(DWORD idThread,UINT Msg,WPARAM wParam,LPARAM lParam) TRAMPOLINE_DEF
#define PostQuitMessage TrampPostQuitMessage
TRAMPFUNC VOID WINAPI PostQuitMessage(int nExitCode) TRAMPOLINE_DEF_VOID


#define GetMessageA TrampGetMessageA
TRAMPFUNC BOOL WINAPI GetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) TRAMPOLINE_DEF
#define GetMessageW TrampGetMessageW
TRAMPFUNC BOOL WINAPI GetMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) TRAMPOLINE_DEF
#define PeekMessageA TrampPeekMessageA
TRAMPFUNC BOOL WINAPI PeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) TRAMPOLINE_DEF
#define PeekMessageW TrampPeekMessageW
TRAMPFUNC BOOL WINAPI PeekMessageW(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) TRAMPOLINE_DEF
#define GetQueueStatus TrampGetQueueStatus
TRAMPFUNC DWORD WINAPI GetQueueStatus(UINT flags) TRAMPOLINE_DEF
#define GetInputState TrampGetInputState
TRAMPFUNC BOOL WINAPI GetInputState() TRAMPOLINE_DEF
#define GetMessagePos TrampGetMessagePos
TRAMPFUNC DWORD WINAPI GetMessagePos(VOID) TRAMPOLINE_DEF
#define GetMessageTime TrampGetMessageTime
TRAMPFUNC LONG WINAPI GetMessageTime(VOID) TRAMPOLINE_DEF

#define TranslateMessage TrampTranslateMessage
TRAMPFUNC BOOL WINAPI TranslateMessage(CONST MSG *lpMsg) TRAMPOLINE_DEF
#define DispatchMessageA TrampDispatchMessageA
TRAMPFUNC LRESULT WINAPI DispatchMessageA(CONST MSG *lpMsg) TRAMPOLINE_DEF
#define DispatchMessageW TrampDispatchMessageW
TRAMPFUNC LRESULT WINAPI DispatchMessageW(CONST MSG *lpMsg) TRAMPOLINE_DEF

#define RegisterWindowMessageA TrampRegisterWindowMessageA
TRAMPFUNC UINT WINAPI RegisterWindowMessageA(LPCSTR lpString) TRAMPOLINE_DEF
#define RegisterWindowMessageW TrampRegisterWindowMessageW
TRAMPFUNC UINT WINAPI RegisterWindowMessageW(LPCWSTR lpString) TRAMPOLINE_DEF

#define DefWindowProcA TrampDefWindowProcA
TRAMPFUNC LRESULT WINAPI DefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) TRAMPOLINE_DEF
#define DefWindowProcW TrampDefWindowProcW
TRAMPFUNC LRESULT WINAPI DefWindowProcW(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) TRAMPOLINE_DEF

#endif

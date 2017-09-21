/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#include "../intercept.h"
#include "../msgqueue.h"

namespace Hooks
{
    extern bool hasPostedMessages;

    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          SendNotifyMessageA,
                          HWND hWnd,
                          UINT Msg,
                          WPARAM wParam,
                          LPARAM lParam);
    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          SendNotifyMessageW,
                          HWND hWnd,
                          UINT Msg,
                          WPARAM wParam,
                          LPARAM lParam);
    HOOK_FUNCTION_DECLARE(LRESULT,
                          WINAPI,
                          SendMessageTimeoutA,
                          HWND hWnd,
                          UINT Msg,
                          WPARAM wParam,
                          LPARAM lParam,
                          UINT fuFlags,
                          UINT uTimeout,
                          PDWORD_PTR lpdwResult);
    HOOK_FUNCTION_DECLARE(LRESULT,
                          WINAPI,
                          SendMessageTimeoutW,
                          HWND hWnd,
                          UINT Msg,
                          WPARAM wParam,
                          LPARAM lParam,
                          UINT fuFlags,
                          UINT uTimeout,
                          PDWORD_PTR lpdwResult);
    HOOK_FUNCTION_DECLARE(LRESULT,
                          WINAPI,
                          SendMessageCallbackA,
                          HWND hWnd,
                          UINT Msg,
                          WPARAM wParam,
                          LPARAM lParam,
                          SENDASYNCPROC lpResultCallBack,
                          ULONG_PTR dwData);
    HOOK_FUNCTION_DECLARE(LRESULT,
                          WINAPI,
                          SendMessageCallbackW,
                          HWND hWnd,
                          UINT Msg,
                          WPARAM wParam,
                          LPARAM lParam,
                          SENDASYNCPROC lpResultCallBack,
                          ULONG_PTR dwData);
    HOOK_FUNCTION_DECLARE(LRESULT,
                          WINAPI,
                          SendMessageW,
                          HWND hWnd,
                          UINT Msg,
                          WPARAM wParam,
                          LPARAM lParam);
    HOOK_FUNCTION_DECLARE(LRESULT,
                          WINAPI,
                          SendMessageA,
                          HWND hWnd,
                          UINT Msg,
                          WPARAM wParam,
                          LPARAM lParam);
    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          PostMessageW,
                          HWND hWnd,
                          UINT Msg,
                          WPARAM wParam,
                          LPARAM lParam);
    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          PostMessageA,
                          HWND hWnd,
                          UINT Msg,
                          WPARAM wParam,
                          LPARAM lParam);
    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          PostThreadMessageA,
                          DWORD idThread,
                          UINT Msg,
                          WPARAM wParam,
                          LPARAM lParam);
    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          PostThreadMessageW,
                          DWORD idThread,
                          UINT Msg,
                          WPARAM wParam,
                          LPARAM lParam);
    HOOK_FUNCTION_DECLARE(VOID, WINAPI, PostQuitMessage, int nExitCode);

    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          GetMessageA,
                          LPMSG lpMsg,
                          HWND hWnd,
                          UINT wMsgFilterMin,
                          UINT wMsgFilterMax);
    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          GetMessageW,
                          LPMSG lpMsg,
                          HWND hWnd,
                          UINT wMsgFilterMin,
                          UINT wMsgFilterMax);
    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          PeekMessageA,
                          LPMSG lpMsg,
                          HWND hWnd,
                          UINT wMsgFilterMin,
                          UINT wMsgFilterMax,
                          UINT wRemoveMsg);
    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          PeekMessageW,
                          LPMSG lpMsg,
                          HWND hWnd,
                          UINT wMsgFilterMin,
                          UINT wMsgFilterMax,
                          UINT wRemoveMsg);
    HOOK_FUNCTION_DECLARE(DWORD, WINAPI, GetQueueStatus, UINT flags);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, GetInputState);
    HOOK_FUNCTION_DECLARE(DWORD, WINAPI, GetMessagePos);
    HOOK_FUNCTION_DECLARE(LONG, WINAPI, GetMessageTime);

    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, TranslateMessage, CONST MSG* lpMsg);
    HOOK_FUNCTION_DECLARE(LRESULT, WINAPI, DispatchMessageA, CONST MSG* lpMsg);
    HOOK_FUNCTION_DECLARE(LRESULT, WINAPI, DispatchMessageW, CONST MSG* lpMsg);

    HOOK_FUNCTION_DECLARE(UINT, WINAPI, RegisterWindowMessageA, LPCSTR lpString);
    HOOK_FUNCTION_DECLARE(UINT, WINAPI, RegisterWindowMessageW, LPCWSTR lpString);

    HOOK_FUNCTION_DECLARE(LRESULT,
                          WINAPI,
                          DefWindowProcA,
                          HWND hWnd,
                          UINT Msg,
                          WPARAM wParam,
                          LPARAM lParam);
    HOOK_FUNCTION_DECLARE(LRESULT,
                          WINAPI,
                          DefWindowProcW,
                          HWND hWnd,
                          UINT Msg,
                          WPARAM wParam,
                          LPARAM lParam);

    LRESULT CALLBACK MyWndProcA(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK MyWndProcW(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT DispatchMessageInternal(HWND hWnd,
                                    UINT message,
                                    WPARAM wParam,
                                    LPARAM lParam,
                                    bool ascii = true,
                                    MessageActionFlags maf = MAF_PASSTHROUGH | MAF_RETURN_OS);
    void PostMessageInternal(HWND hWnd,
                             UINT message,
                             WPARAM wParam,
                             LPARAM lParam,
                             bool ascii = true,
                             struct MessageQueue* pmq = NULL,
                             MessageActionFlags maf = MAF_PASSTHROUGH | MAF_RETURN_OS);
    void HandlePostedMessages();

    void ApplyMessageIntercepts();

    void MessageDllMainInit();
}

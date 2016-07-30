/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#include "../intercept.h"

namespace Hooks
{
    HOOK_FUNCTION_DECLARE(MMRESULT, WINAPI, timeSetEvent, UINT uDelay, UINT uResolution, LPTIMECALLBACK lpTimeProc, DWORD_PTR dwUser, UINT fuEvent);
    HOOK_FUNCTION_DECLARE(MMRESULT, WINAPI, timeKillEvent, UINT uTimerID);
    HOOK_FUNCTION_DECLARE(UINT_PTR, WINAPI, SetTimer, HWND hWnd, UINT_PTR nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, KillTimer, HWND hWnd, UINT_PTR nIDEvent);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, CreateTimerQueueTimer, PHANDLE phNewTimer, HANDLE TimerQueue, WAITORTIMERCALLBACKFUNC Callback, PVOID Parameter, DWORD DueTime, DWORD Period, ULONG Flags);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, SetWaitableTimer, HANDLE hTimer, const LARGE_INTEGER *lpDueTime, LONG lPeriod, PTIMERAPCROUTINE pfnCompletionRoutine, LPVOID lpArgToCompletionRoutine, BOOL fResume);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, CancelWaitableTimer, HANDLE hTimer);

    void ProcessTimers();

    void ApplyTimerIntercepts();

    void TimerDllMainInit();
}

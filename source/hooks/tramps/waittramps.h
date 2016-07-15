/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#include "../intercept.h"

namespace Hooks
{
    HOOK_DECLARE(NTSTATUS, NTAPI, NtWaitForSingleObject, HANDLE Handle, BOOLEAN Alertable, PLARGE_INTEGER Timeout);
    HOOK_DECLARE(NTSTATUS, NTAPI, NtWaitForMultipleObjects, ULONG ObjectCount, PHANDLE ObjectsArray, DWORD WaitType, BOOLEAN Alertable, PLARGE_INTEGER TimeOut);
    HOOK_DECLARE(NTSTATUS, NTAPI, RtlEnterCriticalSection, RTL_CRITICAL_SECTION *crit);
    HOOK_DECLARE(BOOL, NTAPI, RtlTryEnterCriticalSection, RTL_CRITICAL_SECTION* crit);

    HOOK_DECLARE(DWORD, WINAPI, WaitForSingleObject, HANDLE hHandle, DWORD dwMilliseconds);
    HOOK_DECLARE(DWORD, WINAPI, WaitForSingleObjectEx, HANDLE hHandle, DWORD dwMilliseconds, BOOL bAlertable);
    HOOK_DECLARE(DWORD, WINAPI, WaitForMultipleObjects, DWORD nCount, CONST HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds);
    HOOK_DECLARE(DWORD, WINAPI, WaitForMultipleObjectsEx, DWORD nCount, CONST HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds, BOOL bAlertable);
    HOOK_DECLARE(DWORD, WINAPI, MsgWaitForMultipleObjects, DWORD nCount, const HANDLE *pHandles, BOOL bWaitAll, DWORD dwMilliseconds, DWORD dwWakeMask);
    HOOK_DECLARE(DWORD, WINAPI, MsgWaitForMultipleObjectsEx, DWORD nCount, const HANDLE *pHandles, DWORD dwMilliseconds, DWORD dwWakeMask, DWORD dwFlags);
    HOOK_DECLARE(DWORD, WINAPI, SignalObjectAndWait, HANDLE hObjectToSignal, HANDLE hObjectToWaitOn, DWORD dwMilliseconds, BOOL bAlertable);

    HOOK_DECLARE(BOOL, WINAPI, WaitMessage);

    HOOK_DECLARE(VOID, WINAPI, Sleep, DWORD dwMilliseconds);
    HOOK_DECLARE(VOID, WINAPI, SleepEx, DWORD dwMilliseconds, BOOL bAlertable);
    // maybe I should hook NtDelayExecution but that could cause problems and this seems fine...


    // two hacks because:
    // - Sleep calls SleepEx and both are hooked and I want to be able to call Sleep locally
    // - there's a place where I want to call Sleep before it's been hooked
    VOID WINAPI UntrampedSleep(DWORD x);

    void ApplyWaitIntercepts();
}

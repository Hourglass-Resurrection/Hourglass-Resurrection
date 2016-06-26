/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#define NtWaitForSingleObject TrampNtWaitForSingleObject
TRAMPFUNC NTSTATUS NTAPI NtWaitForSingleObject(HANDLE Handle, BOOLEAN Alertable, PLARGE_INTEGER Timeout);
#define NtWaitForMultipleObjects TrampNtWaitForMultipleObjects
TRAMPFUNC NTSTATUS NTAPI NtWaitForMultipleObjects(ULONG ObjectCount, PHANDLE ObjectsArray, DWORD WaitType, BOOLEAN Alertable, PLARGE_INTEGER TimeOut);
#define RtlEnterCriticalSection TrampRtlEnterCriticalSection
TRAMPFUNC NTSTATUS NTAPI RtlEnterCriticalSection( RTL_CRITICAL_SECTION *crit );
#define RtlTryEnterCriticalSection TrampRtlTryEnterCriticalSection
TRAMPFUNC BOOL NTAPI RtlTryEnterCriticalSection(RTL_CRITICAL_SECTION* crit);

#define WaitForSingleObject TrampWaitForSingleObject
TRAMPFUNC DWORD WINAPI WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
#define WaitForSingleObjectEx TrampWaitForSingleObjectEx
TRAMPFUNC DWORD WINAPI WaitForSingleObjectEx(HANDLE hHandle, DWORD dwMilliseconds, BOOL bAlertable);
#define WaitForMultipleObjects TrampWaitForMultipleObjects
TRAMPFUNC DWORD WINAPI WaitForMultipleObjects(DWORD nCount, CONST HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds);
#define WaitForMultipleObjectsEx TrampWaitForMultipleObjectsEx
TRAMPFUNC DWORD WINAPI WaitForMultipleObjectsEx(DWORD nCount, CONST HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds, BOOL bAlertable);
#define MsgWaitForMultipleObjects TrampMsgWaitForMultipleObjects
TRAMPFUNC DWORD WINAPI MsgWaitForMultipleObjects(DWORD nCount, const HANDLE *pHandles, BOOL bWaitAll, DWORD dwMilliseconds, DWORD dwWakeMask);
#define MsgWaitForMultipleObjectsEx TrampMsgWaitForMultipleObjectsEx
TRAMPFUNC DWORD WINAPI MsgWaitForMultipleObjectsEx(DWORD nCount, const HANDLE *pHandles, DWORD dwMilliseconds, DWORD dwWakeMask, DWORD dwFlags);
#define SignalObjectAndWait TrampSignalObjectAndWait
TRAMPFUNC DWORD WINAPI SignalObjectAndWait(HANDLE hObjectToSignal, HANDLE hObjectToWaitOn, DWORD dwMilliseconds, BOOL bAlertable);

#define WaitMessage TrampWaitMessage
TRAMPFUNC BOOL WINAPI WaitMessage();

#define Sleep TrampSleep
TRAMPFUNC VOID WINAPI Sleep(DWORD dwMilliseconds);
#define SleepEx TrampSleepEx
TRAMPFUNC VOID WINAPI SleepEx(DWORD dwMilliseconds, BOOL bAlertable);
// maybe I should hook NtDelayExecution but that could cause problems and this seems fine...


// two hacks because:
// - Sleep calls SleepEx and both are hooked and I want to be able to call Sleep locally
// - there's a place where I want to call Sleep before it's been hooked
#undef Sleep
VOID WINAPI UntrampedSleep(DWORD x);
#define Sleep(x) TrampSleepEx(x,0)

/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef WAITTRAMPS_H_INCL
#define WAITTRAMPS_H_INCL

#define NtWaitForSingleObject TrampNtWaitForSingleObject
TRAMPFUNC NTSTATUS NTAPI NtWaitForSingleObject(HANDLE Handle, BOOLEAN Alertable, PLARGE_INTEGER Timeout) TRAMPOLINE_DEF
#define NtWaitForMultipleObjects TrampNtWaitForMultipleObjects
TRAMPFUNC NTSTATUS NTAPI NtWaitForMultipleObjects(ULONG ObjectCount, PHANDLE ObjectsArray, DWORD WaitType, BOOLEAN Alertable, PLARGE_INTEGER TimeOut) TRAMPOLINE_DEF
#define RtlEnterCriticalSection TrampRtlEnterCriticalSection
TRAMPFUNC NTSTATUS NTAPI RtlEnterCriticalSection( RTL_CRITICAL_SECTION *crit ) TRAMPOLINE_DEF
#define RtlTryEnterCriticalSection TrampRtlTryEnterCriticalSection
TRAMPFUNC BOOL NTAPI RtlTryEnterCriticalSection(RTL_CRITICAL_SECTION* crit) TRAMPOLINE_DEF

#define WaitForSingleObject TrampWaitForSingleObject
TRAMPFUNC DWORD WINAPI WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds) TRAMPOLINE_DEF
#define WaitForSingleObjectEx TrampWaitForSingleObjectEx
TRAMPFUNC DWORD WINAPI WaitForSingleObjectEx(HANDLE hHandle, DWORD dwMilliseconds, BOOL bAlertable) TRAMPOLINE_DEF
#define WaitForMultipleObjects TrampWaitForMultipleObjects
TRAMPFUNC DWORD WINAPI WaitForMultipleObjects(DWORD nCount, CONST HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds) TRAMPOLINE_DEF
#define WaitForMultipleObjectsEx TrampWaitForMultipleObjectsEx
TRAMPFUNC DWORD WINAPI WaitForMultipleObjectsEx(DWORD nCount, CONST HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds, BOOL bAlertable) TRAMPOLINE_DEF
#define MsgWaitForMultipleObjects TrampMsgWaitForMultipleObjects
TRAMPFUNC DWORD WINAPI MsgWaitForMultipleObjects(DWORD nCount, const HANDLE *pHandles, BOOL bWaitAll, DWORD dwMilliseconds, DWORD dwWakeMask) TRAMPOLINE_DEF
#define MsgWaitForMultipleObjectsEx TrampMsgWaitForMultipleObjectsEx
TRAMPFUNC DWORD WINAPI MsgWaitForMultipleObjectsEx(DWORD nCount, const HANDLE *pHandles, DWORD dwMilliseconds, DWORD dwWakeMask, DWORD dwFlags) TRAMPOLINE_DEF
#define SignalObjectAndWait TrampSignalObjectAndWait
TRAMPFUNC DWORD WINAPI SignalObjectAndWait(HANDLE hObjectToSignal, HANDLE hObjectToWaitOn, DWORD dwMilliseconds, BOOL bAlertable) TRAMPOLINE_DEF

#define WaitMessage TrampWaitMessage
TRAMPFUNC BOOL WINAPI WaitMessage() TRAMPOLINE_DEF

#define Sleep TrampSleep
TRAMPFUNC VOID WINAPI Sleep(DWORD dwMilliseconds) TRAMPOLINE_DEF_VOID
#define SleepEx TrampSleepEx
TRAMPFUNC VOID WINAPI SleepEx(DWORD dwMilliseconds, BOOL bAlertable) TRAMPOLINE_DEF_VOID
// maybe I should hook NtDelayExecution but that could cause problems and this seems fine...


// two hacks because:
// - Sleep calls SleepEx and both are hooked and I want to be able to call Sleep locally
// - there's a place where I want to call Sleep before it's been hooked
#undef Sleep
VOID WINAPI UntrampedSleep(DWORD x) TRAMPOLINE_DEF_CUSTOM(Sleep(x))
#define Sleep(x) TrampSleepEx(x,0)


#endif

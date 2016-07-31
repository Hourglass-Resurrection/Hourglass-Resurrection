/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#include "../global.h"
#include "shared/ipc.h"

#include <algorithm>
#include <map>
#include <set>

using Log = DebugLog<LogCategory::SYNC>;

namespace Hooks
{
    static std::map<DWORD, std::set<HANDLE> > s_threadIdHandles;
    static CRITICAL_SECTION s_handleCS;

    //static std::map<DWORD,HANDLE> s_threadHandleToFakeThreadHandle;


    void CloseHandles(DWORD threadId)
    {
        EnterCriticalSection(&s_handleCS);
        std::set<HANDLE>& handles = s_threadIdHandles[threadId];
        std::set<HANDLE>::iterator iter;
        for (iter = handles.begin(); iter != handles.end(); iter++)
        {
            HANDLE handle = *iter;
            DEBUG_LOG() << "CLOSING HANDLE: " << handle;
            CloseHandle(handle);
        }
        handles.clear();
        LeaveCriticalSection(&s_handleCS);
    }

    HOOK_FUNCTION(HANDLE, WINAPI, CreateEventA,
        LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName);
    HOOKFUNC HANDLE WINAPI MyCreateEventA(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName)
    {
        ENTER();
        // TODO: disabling this makes the DirectMusic timer somewhat saveable... what drives that timer??
        HANDLE rv = CreateEventA(lpEventAttributes, bManualReset, bInitialState, lpName);
        LEAVE(rv);
        //verbosedebugprintf("%d %d %s\n", bManualReset, bInitialState, lpName);
        EnterCriticalSection(&s_handleCS);
        std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
        handles.insert(rv);
        LeaveCriticalSection(&s_handleCS);
        return rv;
    }
    HOOK_FUNCTION(HANDLE, WINAPI, CreateEventW,
        LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName);
    HOOKFUNC HANDLE WINAPI MyCreateEventW(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName)
    {
        ENTER();
        HANDLE rv = CreateEventW(lpEventAttributes, bManualReset, bInitialState, lpName);
        LEAVE(rv);
        EnterCriticalSection(&s_handleCS);
        std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
        handles.insert(rv);
        //for(unsigned int i = 0; i < handles.size(); i++)
        //	debugprintf(" . 0x%X . ", handles[i]);
        LeaveCriticalSection(&s_handleCS);
        return rv;
    }
    HOOK_FUNCTION(HANDLE, WINAPI, OpenEventA, DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName);
    HOOKFUNC HANDLE WINAPI MyOpenEventA(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName)
    {
        ENTER();
        HANDLE rv = OpenEventA(dwDesiredAccess, bInheritHandle, lpName);
        LEAVE(rv);
        EnterCriticalSection(&s_handleCS);
        std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
        handles.insert(rv);
        LeaveCriticalSection(&s_handleCS);
        return rv;
    }
    HOOK_FUNCTION(HANDLE, WINAPI, OpenEventW, DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName);
    HOOKFUNC HANDLE WINAPI MyOpenEventW(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName)
    {
        ENTER();
        HANDLE rv = OpenEventW(dwDesiredAccess, bInheritHandle, lpName);
        LEAVE(rv);
        EnterCriticalSection(&s_handleCS);
        std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
        handles.insert(rv);
        LeaveCriticalSection(&s_handleCS);
        return rv;
    }
    HOOK_FUNCTION(BOOL, WINAPI, SetEvent, HANDLE hEvent);
    HOOKFUNC BOOL WINAPI MySetEvent(HANDLE hEvent)
    {
        ENTER(hEvent);
        BOOL rv = SetEvent(hEvent);
        return rv;
    }
    HOOK_FUNCTION(BOOL, WINAPI, ResetEvent, HANDLE hEvent);
    HOOKFUNC BOOL WINAPI MyResetEvent(HANDLE hEvent)
    {
        ENTER(hEvent);
        BOOL rv = ResetEvent(hEvent);
        return rv;
    }

    HOOK_FUNCTION(HANDLE, WINAPI, CreateMutexA, LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCSTR lpName);
    HOOKFUNC HANDLE WINAPI MyCreateMutexA(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCSTR lpName)
    {
        ENTER();
        HANDLE rv = CreateMutexA(lpMutexAttributes, bInitialOwner, lpName);
        LEAVE(rv);
        EnterCriticalSection(&s_handleCS);
        std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
        handles.insert(rv);
        LeaveCriticalSection(&s_handleCS);
        return rv;
    }
    HOOK_FUNCTION(HANDLE, WINAPI, OpenMutexA, DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName);
    HOOKFUNC HANDLE WINAPI MyOpenMutexA(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName)
    {
        ENTER();
        HANDLE rv = OpenMutexA(dwDesiredAccess, bInheritHandle, lpName);
        LEAVE(rv);
        EnterCriticalSection(&s_handleCS);
        std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
        handles.insert(rv);
        LeaveCriticalSection(&s_handleCS);
        return rv;
    }
    HOOK_FUNCTION(HANDLE, WINAPI, CreateMutexW, LPSECURITY_ATTRIBUTES lpMutexWttributes, BOOL bInitialOwner, LPCSTR lpName);
    HOOKFUNC HANDLE WINAPI MyCreateMutexW(LPSECURITY_ATTRIBUTES lpMutexWttributes, BOOL bInitialOwner, LPCSTR lpName)
    {
        ENTER();
        HANDLE rv = CreateMutexW(lpMutexWttributes, bInitialOwner, lpName);
        LEAVE(rv);
        EnterCriticalSection(&s_handleCS);
        std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
        handles.insert(rv);
        LeaveCriticalSection(&s_handleCS);
        return rv;
    }
    HOOK_FUNCTION(HANDLE, WINAPI, OpenMutexW, DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName);
    HOOKFUNC HANDLE WINAPI MyOpenMutexW(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName)
    {
        ENTER();
        HANDLE rv = OpenMutexW(dwDesiredAccess, bInheritHandle, lpName);
        LEAVE(rv);
        EnterCriticalSection(&s_handleCS);
        std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
        handles.insert(rv);
        LeaveCriticalSection(&s_handleCS);
        return rv;
    }
    HOOK_FUNCTION(BOOL, WINAPI, ReleaseMutex, HANDLE hMutex);
    HOOKFUNC BOOL WINAPI MyReleaseMutex(HANDLE hMutex)
    {
        ENTER();

        BOOL rv = ReleaseMutex(hMutex);
        return rv;
    }


    HOOK_FUNCTION(BOOL, WINAPI, CloseHandle, HANDLE hObject);
    HOOKFUNC BOOL WINAPI MyCloseHandle(HANDLE hObject)
    {
        ENTER(hObject);
        BOOL rv = TRUE;
        //	if(threadWrappersOriginalHandleToId.find(hObject) == threadWrappersOriginalHandleToId.end())
        {
            rv = CloseHandle(hObject);
        }
#if 0
        EnterCriticalSection(&s_handleCS);
        std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
        handles.erase(hObject);
        //std::vector<HANDLE>& handles = s_threadIdHandles[threadId];
        //////handles.erase(std::remove(handles.begin(), handles.end(), hObject));
        ////for(unsigned int i = 0; i < handles.size(); i++)
        ////	if(handles[i] == hObject)
        ////		handles[i] = NULL;
        //std::vector<HANDLE>::iterator iter = std::find(handles.begin(), handles.end(), hObject);
        //if(iter != handles.end())
        //	handles.erase(iter);
        ////for(unsigned int i = 0; i < handles.size(); i++)
        ////	debugprintf(" . 0x%X . ", handles[i]);
        LeaveCriticalSection(&s_handleCS);
#endif
        return rv;
    }

    HOOK_FUNCTION(BOOL, WINAPI, DuplicateHandle, HANDLE hSourceProcessHandle,
        HANDLE hSourceHandle, HANDLE hTargetProcessHandle, LPHANDLE lpTargetHandle,
        DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwOptions);
    HOOKFUNC BOOL WINAPI MyDuplicateHandle(HANDLE hSourceProcessHandle,
        HANDLE hSourceHandle, HANDLE hTargetProcessHandle, LPHANDLE lpTargetHandle,
        DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwOptions)
    {
        HANDLE handle = 0;
        if (!lpTargetHandle)
            lpTargetHandle = &handle;
        BOOL rv = DuplicateHandle(hSourceProcessHandle, hSourceHandle,
            hTargetProcessHandle, lpTargetHandle,
            dwDesiredAccess, bInheritHandle, dwOptions);

        LOG() << "hSourceHandle " << hSourceHandle << " -> lpTargetHandle " << *lpTargetHandle;

        EnterCriticalSection(&s_handleCS);
        std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
        handles.insert(*lpTargetHandle);
        if (dwOptions & DUPLICATE_CLOSE_SOURCE)
        {
            handles.erase(hSourceProcessHandle);
            //std::vector<HANDLE>::iterator iter = std::find(handles.begin(), handles.end(), hSourceProcessHandle);
            //if(iter != handles.end())
            //	handles.erase(iter);
        }
        ////		handles.erase(std::remove(handles.begin(), handles.end(), hSourceProcessHandle));
        //		for(unsigned int i = 0; i < handles.size(); i++)
        //			if(handles[i] == hSourceProcessHandle)
        //				handles[i] = NULL;
        LeaveCriticalSection(&s_handleCS);

        return rv;
    }



    HOOK_FUNCTION(HANDLE, WINAPI, CreateSemaphoreA, LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCSTR lpName);
    HOOKFUNC HANDLE WINAPI MyCreateSemaphoreA(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCSTR lpName)
    {
        ENTER();
        HANDLE rv = CreateSemaphoreA(lpSemaphoreAttributes, lInitialCount, lMaximumCount, lpName);
        LEAVE(rv);
        EnterCriticalSection(&s_handleCS);
        std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
        handles.insert(rv);
        LeaveCriticalSection(&s_handleCS);
        return rv;
    }
    HOOK_FUNCTION(HANDLE, WINAPI, CreateSemaphoreW, LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCWSTR lpName);
    HOOKFUNC HANDLE WINAPI MyCreateSemaphoreW(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCWSTR lpName)
    {
        ENTER();
        HANDLE rv = CreateSemaphoreW(lpSemaphoreAttributes, lInitialCount, lMaximumCount, lpName);
        LEAVE(rv);
        EnterCriticalSection(&s_handleCS);
        std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
        handles.insert(rv);
        LeaveCriticalSection(&s_handleCS);
        return rv;
    }
    HOOK_FUNCTION(HANDLE, WINAPI, OpenSemaphoreA, DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName);
    HOOKFUNC HANDLE WINAPI MyOpenSemaphoreA(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName)
    {
        ENTER();
        HANDLE rv = OpenSemaphoreA(dwDesiredAccess, bInheritHandle, lpName);
        LEAVE(rv);
        EnterCriticalSection(&s_handleCS);
        std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
        handles.insert(rv);
        LeaveCriticalSection(&s_handleCS);
        return rv;
    }
    HOOK_FUNCTION(HANDLE, WINAPI, OpenSemaphoreW, DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName);
    HOOKFUNC HANDLE WINAPI MyOpenSemaphoreW(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName)
    {
        ENTER();
        HANDLE rv = OpenSemaphoreW(dwDesiredAccess, bInheritHandle, lpName);
        LEAVE(rv);
        EnterCriticalSection(&s_handleCS);
        std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
        handles.insert(rv);
        LeaveCriticalSection(&s_handleCS);
        return rv;
    }
    HOOK_FUNCTION(HANDLE, WINAPI, CreateWaitableTimerA, LPSECURITY_ATTRIBUTES lpTimerAttributes, BOOL bManualReset, LPCSTR lpTimerName);
    HOOKFUNC HANDLE WINAPI MyCreateWaitableTimerA(LPSECURITY_ATTRIBUTES lpTimerAttributes, BOOL bManualReset, LPCSTR lpTimerName)
    {
        ENTER();
        HANDLE rv = CreateWaitableTimerA(lpTimerAttributes, bManualReset, lpTimerName);
        LEAVE(rv);
        EnterCriticalSection(&s_handleCS);
        std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
        handles.insert(rv);
        LeaveCriticalSection(&s_handleCS);
        return rv;
    }
    HOOK_FUNCTION(HANDLE, WINAPI, CreateWaitableTimerW, LPSECURITY_ATTRIBUTES lpTimerAttributes, BOOL bManualReset, LPCWSTR lpTimerName);
    HOOKFUNC HANDLE WINAPI MyCreateWaitableTimerW(LPSECURITY_ATTRIBUTES lpTimerAttributes, BOOL bManualReset, LPCWSTR lpTimerName)
    {
        ENTER();
        HANDLE rv = CreateWaitableTimerW(lpTimerAttributes, bManualReset, lpTimerName);
        LEAVE(rv);
        EnterCriticalSection(&s_handleCS);
        std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
        handles.insert(rv);
        LeaveCriticalSection(&s_handleCS);
        return rv;
    }
    HOOK_FUNCTION(HANDLE, WINAPI, OpenWaitableTimerA, DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpTimerName);
    HOOKFUNC HANDLE WINAPI MyOpenWaitableTimerA(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpTimerName)
    {
        ENTER();
        HANDLE rv = OpenWaitableTimerA(dwDesiredAccess, bInheritHandle, lpTimerName);
        LEAVE(rv);
        EnterCriticalSection(&s_handleCS);
        std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
        handles.insert(rv);
        LeaveCriticalSection(&s_handleCS);
        return rv;
    }
    HOOK_FUNCTION(HANDLE, WINAPI, OpenWaitableTimerW, DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpTimerName);
    HOOKFUNC HANDLE WINAPI MyOpenWaitableTimerW(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpTimerName)
    {
        ENTER();
        HANDLE rv = OpenWaitableTimerW(dwDesiredAccess, bInheritHandle, lpTimerName);
        LEAVE(rv);
        EnterCriticalSection(&s_handleCS);
        std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
        handles.insert(rv);
        LeaveCriticalSection(&s_handleCS);
        return rv;
    }
    HOOK_FUNCTION(BOOL, WINAPI, SetWaitableTimer, HANDLE hTimer, const LARGE_INTEGER *lpDueTime, LONG lPeriod, PTIMERAPCROUTINE pfnCompletionRoutine, LPVOID lpArgToCompletionRoutine, BOOL fResume);
    HOOKFUNC BOOL WINAPI MySetWaitableTimer(HANDLE hTimer, const LARGE_INTEGER *lpDueTime, LONG lPeriod, PTIMERAPCROUTINE pfnCompletionRoutine, LPVOID lpArgToCompletionRoutine, BOOL fResume)
    {
        ENTER();
        BOOL rv = SetWaitableTimer(hTimer, lpDueTime, lPeriod, pfnCompletionRoutine, lpArgToCompletionRoutine, fResume);
        LEAVE(rv);
        return rv;
    }
    HOOK_FUNCTION(BOOL, WINAPI, CancelWaitableTimer, HANDLE hTimer);
    HOOKFUNC BOOL WINAPI MyCancelWaitableTimer(HANDLE hTimer)
    {
        ENTER();
        BOOL rv = CancelWaitableTimer(hTimer);
        LEAVE(rv);
        return rv;
    }

    HOOK_FUNCTION(DWORD, WINAPI, QueueUserAPC, PAPCFUNC pfnAPC, HANDLE hThread, ULONG_PTR dwData);
    HOOKFUNC DWORD WINAPI MyQueueUserAPC(PAPCFUNC pfnAPC, HANDLE hThread, ULONG_PTR dwData)
    {
        ENTER(pfnAPC, hThread, dwData);
        DWORD rv = QueueUserAPC(pfnAPC, hThread, dwData);
        LEAVE(rv);
        return rv;
    }

    void SyncDllMainInit()
    {
        InitializeCriticalSection(&s_handleCS);
    }

    void ApplySyncIntercepts()
    {
        static const InterceptDescriptor intercepts[] =
        {
            MAKE_INTERCEPT(/*1*/0, KERNEL32, CreateMutexW),
            MAKE_INTERCEPT(/*1*/0, KERNEL32, OpenMutexW),
            MAKE_INTERCEPT(/*1*/0, KERNEL32, CreateEventW),
            MAKE_INTERCEPT(/*1*/0, KERNEL32, OpenEventW),
            MAKE_INTERCEPT(0, KERNEL32, CreateMutexA),
            MAKE_INTERCEPT(0, KERNEL32, OpenMutexA),
            MAKE_INTERCEPT(0, KERNEL32, CreateEventA),
            MAKE_INTERCEPT(0, KERNEL32, OpenEventA),
            MAKE_INTERCEPT(0, KERNEL32, ReleaseMutex),
            MAKE_INTERCEPT(0, KERNEL32, CreateSemaphoreA),
            MAKE_INTERCEPT(0, KERNEL32, CreateSemaphoreW),
            MAKE_INTERCEPT(0, KERNEL32, OpenSemaphoreA),
            MAKE_INTERCEPT(0, KERNEL32, OpenSemaphoreW),
            MAKE_INTERCEPT(0, KERNEL32, CreateWaitableTimerA),
            MAKE_INTERCEPT(0, KERNEL32, CreateWaitableTimerW),
            MAKE_INTERCEPT(0, KERNEL32, OpenWaitableTimerA),
            MAKE_INTERCEPT(0, KERNEL32, OpenWaitableTimerW),
            MAKE_INTERCEPT(0, KERNEL32, SetWaitableTimer),
            MAKE_INTERCEPT(0, KERNEL32, CancelWaitableTimer),
            MAKE_INTERCEPT(0, KERNEL32, SetEvent),
            MAKE_INTERCEPT(0, KERNEL32, ResetEvent),
            MAKE_INTERCEPT(/*1*/0, KERNEL32, DuplicateHandle),
            MAKE_INTERCEPT(/*1*/0, KERNEL32, CloseHandle),
            MAKE_INTERCEPT(1, KERNEL32, QueueUserAPC),
        };
        ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
    }
}

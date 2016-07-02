/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#include "../intercept.h"

namespace Hooks
{
    HOOK_DECLARE(HANDLE, WINAPI, CreateEventA, LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName);
    HOOK_DECLARE(HANDLE, WINAPI, CreateEventW, LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName);
    HOOK_DECLARE(HANDLE, WINAPI, OpenEventA, DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName);
    HOOK_DECLARE(HANDLE, WINAPI, OpenEventW, DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName);
    HOOK_DECLARE(BOOL, WINAPI, SetEvent, HANDLE hEvent);
    HOOK_DECLARE(BOOL, WINAPI, ResetEvent, HANDLE hEvent);

    HOOK_DECLARE(HANDLE, WINAPI, CreateMutexA, LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCSTR lpName);
    HOOK_DECLARE(HANDLE, WINAPI, OpenMutexA, DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName);
    HOOK_DECLARE(HANDLE, WINAPI, CreateMutexW, LPSECURITY_ATTRIBUTES lpMutexWttributes, BOOL bInitialOwner, LPCSTR lpName);
    HOOK_DECLARE(HANDLE, WINAPI, OpenMutexW, DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName);
    HOOK_DECLARE(BOOL, WINAPI, ReleaseMutex, HANDLE hMutex);

    HOOK_DECLARE(HANDLE, WINAPI, CreateSemaphoreA, LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCSTR lpName);
    HOOK_DECLARE(HANDLE, WINAPI, CreateSemaphoreW, LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCWSTR lpName);
    HOOK_DECLARE(HANDLE, WINAPI, OpenSemaphoreA, DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName);
    HOOK_DECLARE(HANDLE, WINAPI, OpenSemaphoreW, DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName);
    HOOK_DECLARE(HANDLE, WINAPI, CreateWaitableTimerA, LPSECURITY_ATTRIBUTES lpTimerAttributes, BOOL bManualReset, LPCSTR lpTimerName);
    HOOK_DECLARE(HANDLE, WINAPI, CreateWaitableTimerW, LPSECURITY_ATTRIBUTES lpTimerAttributes, BOOL bManualReset, LPCWSTR lpTimerName);
    HOOK_DECLARE(HANDLE, WINAPI, OpenWaitableTimerA, DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpTimerName);
    HOOK_DECLARE(HANDLE, WINAPI, OpenWaitableTimerW, DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpTimerName);

    // close enough
    HOOK_DECLARE(BOOL, WINAPI, CloseHandle, HANDLE hObject);
    HOOK_DECLARE(BOOL, WINAPI, DuplicateHandle, HANDLE hSourceProcessHandle,
        HANDLE hSourceHandle, HANDLE hTargetProcessHandle, LPHANDLE lpTargetHandle,
        DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwOptions
    );

    void ApplySyncIntercepts();

    void SyncDllMainInit();
}

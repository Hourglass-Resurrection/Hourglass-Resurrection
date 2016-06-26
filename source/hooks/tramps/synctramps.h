/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#define CreateEventA TrampCreateEventA
TRAMPFUNC HANDLE WINAPI CreateEventA(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName);
#define CreateEventW TrampCreateEventW
TRAMPFUNC HANDLE WINAPI CreateEventW(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName);
#define OpenEventA TrampOpenEventA
TRAMPFUNC HANDLE WINAPI OpenEventA(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName);
#define OpenEventW TrampOpenEventW
TRAMPFUNC HANDLE WINAPI OpenEventW(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName);
#define SetEvent TrampSetEvent
TRAMPFUNC BOOL WINAPI SetEvent(HANDLE hEvent);
#define ResetEvent TrampResetEvent
TRAMPFUNC BOOL WINAPI ResetEvent(HANDLE hEvent);

#define CreateMutexA TrampCreateMutexA
TRAMPFUNC HANDLE WINAPI CreateMutexA(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCSTR lpName);
#define OpenMutexA TrampOpenMutexA
TRAMPFUNC HANDLE WINAPI OpenMutexA(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName);
#define CreateMutexW TrampCreateMutexW
TRAMPFUNC HANDLE WINAPI CreateMutexW(LPSECURITY_ATTRIBUTES lpMutexWttributes, BOOL bInitialOwner, LPCSTR lpName);
#define OpenMutexW TrampOpenMutexW
TRAMPFUNC HANDLE WINAPI OpenMutexW(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName);
#define ReleaseMutex TrampReleaseMutex
TRAMPFUNC BOOL WINAPI ReleaseMutex(HANDLE hMutex);

#define CreateSemaphoreA TrampCreateSemaphoreA
TRAMPFUNC HANDLE WINAPI CreateSemaphoreA(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,LONG lInitialCount,LONG lMaximumCount,LPCSTR lpName) ;
#define CreateSemaphoreW TrampCreateSemaphoreW
TRAMPFUNC HANDLE WINAPI CreateSemaphoreW(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,LONG lInitialCount,LONG lMaximumCount,LPCWSTR lpName) ;
#define OpenSemaphoreA TrampOpenSemaphoreA
TRAMPFUNC HANDLE WINAPI OpenSemaphoreA(DWORD dwDesiredAccess,BOOL bInheritHandle,LPCSTR lpName) ;
#define OpenSemaphoreW TrampOpenSemaphoreW
TRAMPFUNC HANDLE WINAPI OpenSemaphoreW(DWORD dwDesiredAccess,BOOL bInheritHandle,LPCWSTR lpName) ;
#define CreateWaitableTimerA TrampCreateWaitableTimerA
TRAMPFUNC HANDLE WINAPI CreateWaitableTimerA(LPSECURITY_ATTRIBUTES lpTimerAttributes,BOOL bManualReset,LPCSTR lpTimerName) ;
#define CreateWaitableTimerW TrampCreateWaitableTimerW
TRAMPFUNC HANDLE WINAPI CreateWaitableTimerW(LPSECURITY_ATTRIBUTES lpTimerAttributes,BOOL bManualReset,LPCWSTR lpTimerName) ;
#define OpenWaitableTimerA TrampOpenWaitableTimerA
TRAMPFUNC HANDLE WINAPI OpenWaitableTimerA(DWORD dwDesiredAccess,BOOL bInheritHandle,LPCSTR lpTimerName) ;
#define OpenWaitableTimerW TrampOpenWaitableTimerW
TRAMPFUNC HANDLE WINAPI OpenWaitableTimerW(DWORD dwDesiredAccess,BOOL bInheritHandle,LPCWSTR lpTimerName) ;

// close enough
#define CloseHandle TrampCloseHandle
TRAMPFUNC BOOL WINAPI CloseHandle(HANDLE hObject);
#define DuplicateHandle TrampDuplicateHandle
TRAMPFUNC BOOL WINAPI DuplicateHandle(HANDLE hSourceProcessHandle,
    HANDLE hSourceHandle, HANDLE hTargetProcessHandle, LPHANDLE lpTargetHandle,
    DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwOptions
);

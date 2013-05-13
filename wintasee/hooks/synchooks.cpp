/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(SYNCHOOKS_INCL) && !defined(UNITY_BUILD)
#define SYNCHOOKS_INCL

#include "../global.h"
#include "../../shared/ipc.h"

#include <map>
#include <set>
#include <algorithm>
static std::map<DWORD,std::set<HANDLE> > s_threadIdHandles;
static CRITICAL_SECTION s_handleCS;

//static std::map<DWORD,HANDLE> s_threadHandleToFakeThreadHandle;


void CloseHandles(DWORD threadId)
{
	EnterCriticalSection(&s_handleCS);
	std::set<HANDLE>& handles = s_threadIdHandles[threadId];
	std::set<HANDLE>::iterator iter;
	for(iter = handles.begin(); iter != handles.end(); iter++)
	{
		HANDLE handle = *iter;
		debugprintf("CLOSING HANDLE: 0x%X\n", (void*)handle);
		CloseHandle(handle);
	}
	handles.clear();
	LeaveCriticalSection(&s_handleCS);
}

HOOKFUNC HANDLE WINAPI MyCreateEventA(LPSECURITY_ATTRIBUTES lpEventAttributes,
	BOOL bManualReset, BOOL bInitialState, LPCSTR lpName)
{
	// TODO: disabling this makes the DirectMusic timer somewhat saveable... what drives that timer??
	HANDLE rv = CreateEventA(lpEventAttributes, bManualReset, bInitialState, lpName);
	debuglog(LCF_SYNCOBJ|LCF_TODO, __FUNCTION__ " returned 0x%X.\n", rv);
	//verbosedebugprintf("%d %d %s\n", bManualReset, bInitialState, lpName);
	EnterCriticalSection(&s_handleCS);
	std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
	handles.insert(rv);
	LeaveCriticalSection(&s_handleCS);
	return rv;
}
HOOKFUNC HANDLE WINAPI MyCreateEventW(LPSECURITY_ATTRIBUTES lpEventAttributes,
	BOOL bManualReset, BOOL bInitialState, LPCWSTR lpName)
{
	HANDLE rv = CreateEventW(lpEventAttributes, bManualReset, bInitialState, lpName);
	debuglog(LCF_SYNCOBJ|LCF_TODO, __FUNCTION__ " returned 0x%X.\n", rv);
	EnterCriticalSection(&s_handleCS);
	std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
	handles.insert(rv);
	//for(unsigned int i = 0; i < handles.size(); i++)
	//	debugprintf(" . 0x%X . ", handles[i]);
	LeaveCriticalSection(&s_handleCS);
	return rv;
}
HOOKFUNC HANDLE WINAPI MyOpenEventA(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName)
{
	HANDLE rv = OpenEventA(dwDesiredAccess, bInheritHandle, lpName);
	debuglog(LCF_SYNCOBJ, __FUNCTION__ " returned 0x%X.\n", rv);
	EnterCriticalSection(&s_handleCS);
	std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
	handles.insert(rv);
	LeaveCriticalSection(&s_handleCS);
	return rv;
}
HOOKFUNC HANDLE WINAPI MyOpenEventW(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCWSTR lpName)
{
	HANDLE rv = OpenEventW(dwDesiredAccess, bInheritHandle, lpName);
	debuglog(LCF_SYNCOBJ, __FUNCTION__ " returned 0x%X.\n", rv);
	EnterCriticalSection(&s_handleCS);
	std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
	handles.insert(rv);
	LeaveCriticalSection(&s_handleCS);
	return rv;
}
HOOKFUNC BOOL WINAPI MySetEvent(HANDLE hEvent)
{
	debuglog(LCF_SYNCOBJ, __FUNCTION__ " called.\n");
	BOOL rv = SetEvent(hEvent);
	return rv;
}
HOOKFUNC BOOL WINAPI MyResetEvent(HANDLE hEvent)
{
	debuglog(LCF_SYNCOBJ, __FUNCTION__ " called.\n");
	BOOL rv = ResetEvent(hEvent);
	return rv;
}

HOOKFUNC HANDLE WINAPI MyCreateMutexA(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCSTR lpName)
{
	HANDLE rv = CreateMutexA(lpMutexAttributes, bInitialOwner, lpName);
	debuglog(LCF_SYNCOBJ, __FUNCTION__ " returned 0x%X.\n", rv);
	EnterCriticalSection(&s_handleCS);
	std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
	handles.insert(rv);
	LeaveCriticalSection(&s_handleCS);
	return rv;
}
HOOKFUNC HANDLE WINAPI MyOpenMutexA(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName)
{
	HANDLE rv = OpenMutexA(dwDesiredAccess, bInheritHandle, lpName);
	debuglog(LCF_SYNCOBJ, __FUNCTION__ " returned 0x%X.\n", rv);
	EnterCriticalSection(&s_handleCS);
	std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
	handles.insert(rv);
	LeaveCriticalSection(&s_handleCS);
	return rv;
}
HOOKFUNC HANDLE WINAPI MyCreateMutexW(LPSECURITY_ATTRIBUTES lpMutexWttributes, BOOL bInitialOwner, LPCSTR lpName)
{
	HANDLE rv = CreateMutexW(lpMutexWttributes, bInitialOwner, lpName);
	debuglog(LCF_SYNCOBJ, __FUNCTION__ " returned 0x%X.\n", rv);
	EnterCriticalSection(&s_handleCS);
	std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
	handles.insert(rv);
	LeaveCriticalSection(&s_handleCS);
	return rv;
}
HOOKFUNC HANDLE WINAPI MyOpenMutexW(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName)
{
	HANDLE rv = OpenMutexW(dwDesiredAccess, bInheritHandle, lpName);
	debuglog(LCF_SYNCOBJ, __FUNCTION__ " returned 0x%X.\n", rv);
	EnterCriticalSection(&s_handleCS);
	std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
	handles.insert(rv);
	LeaveCriticalSection(&s_handleCS);
	return rv;
}
HOOKFUNC BOOL WINAPI MyReleaseMutex(HANDLE hMutex)
{
	debuglog(LCF_SYNCOBJ|LCF_TODO, __FUNCTION__ " called.\n");

	BOOL rv = ReleaseMutex(hMutex);
	return rv;
}


HOOKFUNC BOOL WINAPI MyCloseHandle(HANDLE hObject)
{
	debuglog(LCF_SYNCOBJ|LCF_TODO, __FUNCTION__ "(0x%X) called.\n", hObject);
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

HOOKFUNC BOOL WINAPI MyDuplicateHandle(HANDLE hSourceProcessHandle,
    HANDLE hSourceHandle, HANDLE hTargetProcessHandle, LPHANDLE lpTargetHandle,
    DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwOptions)
{
	HANDLE handle = 0;
	if(!lpTargetHandle)
		lpTargetHandle = &handle;
	BOOL rv = DuplicateHandle(hSourceProcessHandle, hSourceHandle,
		hTargetProcessHandle, lpTargetHandle,
	    dwDesiredAccess, bInheritHandle, dwOptions);

	debuglog(LCF_SYNCOBJ, __FUNCTION__ "(0x%X -> 0x%X).\n", hSourceProcessHandle, *lpTargetHandle);

	EnterCriticalSection(&s_handleCS);
	std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
	handles.insert(*lpTargetHandle);
	if(dwOptions & DUPLICATE_CLOSE_SOURCE)
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



HOOKFUNC HANDLE WINAPI MyCreateSemaphoreA(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,LONG lInitialCount,LONG lMaximumCount,LPCSTR lpName)
{
	HANDLE rv = CreateSemaphoreA(lpSemaphoreAttributes, lInitialCount, lMaximumCount, lpName);
	debuglog(LCF_SYNCOBJ, __FUNCTION__ " returned 0x%X.\n", rv);
	EnterCriticalSection(&s_handleCS);
	std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
	handles.insert(rv);
	LeaveCriticalSection(&s_handleCS);
	return rv;
}
HOOKFUNC HANDLE WINAPI MyCreateSemaphoreW(LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,LONG lInitialCount,LONG lMaximumCount,LPCWSTR lpName)
{
	HANDLE rv = CreateSemaphoreW(lpSemaphoreAttributes, lInitialCount, lMaximumCount, lpName);
	debuglog(LCF_SYNCOBJ, __FUNCTION__ " returned 0x%X.\n", rv);
	EnterCriticalSection(&s_handleCS);
	std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
	handles.insert(rv);
	LeaveCriticalSection(&s_handleCS);
	return rv;
}
HOOKFUNC HANDLE WINAPI MyOpenSemaphoreA(DWORD dwDesiredAccess,BOOL bInheritHandle,LPCSTR lpName)
{
	HANDLE rv = OpenSemaphoreA(dwDesiredAccess, bInheritHandle, lpName);
	debuglog(LCF_SYNCOBJ, __FUNCTION__ " returned 0x%X.\n", rv);
	EnterCriticalSection(&s_handleCS);
	std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
	handles.insert(rv);
	LeaveCriticalSection(&s_handleCS);
	return rv;
}
HOOKFUNC HANDLE WINAPI MyOpenSemaphoreW(DWORD dwDesiredAccess,BOOL bInheritHandle,LPCWSTR lpName)
{
	HANDLE rv = OpenSemaphoreW(dwDesiredAccess, bInheritHandle, lpName);
	debuglog(LCF_SYNCOBJ, __FUNCTION__ " returned 0x%X.\n", rv);
	EnterCriticalSection(&s_handleCS);
	std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
	handles.insert(rv);
	LeaveCriticalSection(&s_handleCS);
	return rv;
}
HOOKFUNC HANDLE WINAPI MyCreateWaitableTimerA(LPSECURITY_ATTRIBUTES lpTimerAttributes,BOOL bManualReset,LPCSTR lpTimerName)
{
	HANDLE rv = CreateWaitableTimerA(lpTimerAttributes, bManualReset, lpTimerName);
	debuglog(LCF_SYNCOBJ|LCF_DESYNC|LCF_UNTESTED|LCF_TODO, __FUNCTION__ " returned 0x%X.\n", rv);
	EnterCriticalSection(&s_handleCS);
	std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
	handles.insert(rv);
	LeaveCriticalSection(&s_handleCS);
	return rv;
}
HOOKFUNC HANDLE WINAPI MyCreateWaitableTimerW(LPSECURITY_ATTRIBUTES lpTimerAttributes,BOOL bManualReset,LPCWSTR lpTimerName)
{
	HANDLE rv = CreateWaitableTimerW(lpTimerAttributes, bManualReset, lpTimerName);
	debuglog(LCF_SYNCOBJ|LCF_DESYNC|LCF_UNTESTED|LCF_TODO, __FUNCTION__ " returned 0x%X.\n", rv);
	EnterCriticalSection(&s_handleCS);
	std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
	handles.insert(rv);
	LeaveCriticalSection(&s_handleCS);
	return rv;
}
HOOKFUNC HANDLE WINAPI MyOpenWaitableTimerA(DWORD dwDesiredAccess,BOOL bInheritHandle,LPCSTR lpTimerName)
{
	HANDLE rv = OpenWaitableTimerA(dwDesiredAccess, bInheritHandle, lpTimerName);
	debuglog(LCF_SYNCOBJ|LCF_DESYNC|LCF_UNTESTED|LCF_TODO, __FUNCTION__ " returned 0x%X.\n", rv);
	EnterCriticalSection(&s_handleCS);
	std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
	handles.insert(rv);
	LeaveCriticalSection(&s_handleCS);
	return rv;
}
HOOKFUNC HANDLE WINAPI MyOpenWaitableTimerW(DWORD dwDesiredAccess,BOOL bInheritHandle,LPCWSTR lpTimerName)
{
	HANDLE rv = OpenWaitableTimerW(dwDesiredAccess, bInheritHandle, lpTimerName);
	debuglog(LCF_SYNCOBJ|LCF_DESYNC|LCF_UNTESTED|LCF_TODO, __FUNCTION__ " returned 0x%X.\n", rv);
	EnterCriticalSection(&s_handleCS);
	std::set<HANDLE>& handles = s_threadIdHandles[GetCurrentThreadId()];
	handles.insert(rv);
	LeaveCriticalSection(&s_handleCS);
	return rv;
}
HOOKFUNC BOOL WINAPI MySetWaitableTimer(HANDLE hTimer,const LARGE_INTEGER *lpDueTime,LONG lPeriod,PTIMERAPCROUTINE pfnCompletionRoutine,LPVOID lpArgToCompletionRoutine,BOOL fResume)
{
	BOOL rv = SetWaitableTimer(hTimer, lpDueTime, lPeriod, pfnCompletionRoutine, lpArgToCompletionRoutine, fResume);
	debuglog(LCF_SYNCOBJ|LCF_DESYNC|LCF_UNTESTED|LCF_TODO, __FUNCTION__ " returned %d.\n", rv);
	return rv;
}
HOOKFUNC BOOL WINAPI MyCancelWaitableTimer(HANDLE hTimer)
{
	BOOL rv = CancelWaitableTimer(hTimer);
	debuglog(LCF_SYNCOBJ|LCF_DESYNC|LCF_UNTESTED|LCF_TODO, __FUNCTION__ " returned %d.\n", rv);
	return rv;
}

void SyncDllMainInit()
{
	InitializeCriticalSection(&s_handleCS);
}

void ApplySyncIntercepts()
{
	static const InterceptDescriptor intercepts [] = 
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
	};
	ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
}

#else
#pragma message(__FILE__": (skipped compilation)")
#endif

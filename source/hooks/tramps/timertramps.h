/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#define timeSetEvent TramptimeSetEvent
TRAMPFUNC MMRESULT WINAPI timeSetEvent(UINT uDelay, UINT uResolution, LPTIMECALLBACK lpTimeProc, DWORD_PTR dwUser, UINT fuEvent);
#define timeKillEvent TramptimeKillEvent
TRAMPFUNC MMRESULT WINAPI timeKillEvent(UINT uTimerID);
#define SetTimer TrampSetTimer
TRAMPFUNC UINT_PTR WINAPI SetTimer(HWND hWnd, UINT_PTR nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc);
#define KillTimer TrampKillTimer
TRAMPFUNC BOOL WINAPI KillTimer(HWND hWnd, UINT_PTR nIDEvent);
#define CreateTimerQueueTimer TrampCreateTimerQueueTimer
TRAMPFUNC BOOL WINAPI CreateTimerQueueTimer(PHANDLE phNewTimer, HANDLE TimerQueue, WAITORTIMERCALLBACKFUNC Callback, PVOID Parameter, DWORD DueTime, DWORD Period, ULONG Flags);
#define SetWaitableTimer TrampSetWaitableTimer
TRAMPFUNC BOOL WINAPI SetWaitableTimer(HANDLE hTimer,const LARGE_INTEGER *lpDueTime,LONG lPeriod,PTIMERAPCROUTINE pfnCompletionRoutine,LPVOID lpArgToCompletionRoutine,BOOL fResume);
#define CancelWaitableTimer TrampCancelWaitableTimer
TRAMPFUNC BOOL WINAPI CancelWaitableTimer(HANDLE hTimer);
#define QueueUserAPC TrampQueueUserAPC
TRAMPFUNC DWORD WINAPI QueueUserAPC(PAPCFUNC pfnAPC, HANDLE hThread, ULONG_PTR dwData);

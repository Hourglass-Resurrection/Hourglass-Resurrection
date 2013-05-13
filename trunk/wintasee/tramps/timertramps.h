/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef TIMERTRAMPS_H_INCL
#define TIMERTRAMPS_H_INCL

#define timeSetEvent TramptimeSetEvent
TRAMPFUNC MMRESULT WINAPI timeSetEvent(UINT uDelay, UINT uResolution, LPTIMECALLBACK lpTimeProc, DWORD_PTR dwUser, UINT fuEvent) TRAMPOLINE_DEF
#define timeKillEvent TramptimeKillEvent
TRAMPFUNC MMRESULT WINAPI timeKillEvent(UINT uTimerID) TRAMPOLINE_DEF
#define SetTimer TrampSetTimer
TRAMPFUNC UINT_PTR WINAPI SetTimer(HWND hWnd, UINT_PTR nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc) TRAMPOLINE_DEF
#define KillTimer TrampKillTimer
TRAMPFUNC BOOL WINAPI KillTimer(HWND hWnd, UINT_PTR nIDEvent) TRAMPOLINE_DEF
#define CreateTimerQueueTimer TrampCreateTimerQueueTimer
TRAMPFUNC BOOL WINAPI CreateTimerQueueTimer(PHANDLE phNewTimer, HANDLE TimerQueue, WAITORTIMERCALLBACKFUNC Callback, PVOID Parameter, DWORD DueTime, DWORD Period, ULONG Flags) TRAMPOLINE_DEF
#define SetWaitableTimer TrampSetWaitableTimer
TRAMPFUNC BOOL WINAPI SetWaitableTimer(HANDLE hTimer,const LARGE_INTEGER *lpDueTime,LONG lPeriod,PTIMERAPCROUTINE pfnCompletionRoutine,LPVOID lpArgToCompletionRoutine,BOOL fResume) TRAMPOLINE_DEF
#define CancelWaitableTimer TrampCancelWaitableTimer
TRAMPFUNC BOOL WINAPI CancelWaitableTimer(HANDLE hTimer) TRAMPOLINE_DEF
#define QueueUserAPC TrampQueueUserAPC
TRAMPFUNC DWORD WINAPI QueueUserAPC(PAPCFUNC pfnAPC, HANDLE hThread, ULONG_PTR dwData) TRAMPOLINE_DEF

#endif

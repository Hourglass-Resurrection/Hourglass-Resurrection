/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#define timeGetTime TramptimeGetTime
TRAMPFUNC DWORD WINAPI timeGetTime(void);
#define GetTickCount TrampGetTickCount
TRAMPFUNC DWORD WINAPI GetTickCount(void);
#define GetSystemTimes TrampGetSystemTimes
TRAMPFUNC BOOL WINAPI GetSystemTimes(LPFILETIME lpIdleTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime);
#define timeGetSystemTime TramptimeGetSystemTime
TRAMPFUNC MMRESULT WINAPI timeGetSystemTime(LPMMTIME pmmt, UINT cbmmt);
#define GetSystemTime TrampGetSystemTime
TRAMPFUNC VOID WINAPI GetSystemTime(LPSYSTEMTIME lpSystemTime);
#define GetSystemTimeAsFileTime TrampGetSystemTimeAsFileTime
TRAMPFUNC VOID WINAPI GetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime);
#define GetLocalTime TrampGetLocalTime
TRAMPFUNC VOID WINAPI GetLocalTime(LPSYSTEMTIME lpSystemTime);
#define NtQuerySystemTime TrampNtQuerySystemTime
TRAMPFUNC NTSTATUS NTAPI NtQuerySystemTime(PLARGE_INTEGER SystemTime);
#define QueryPerformanceCounter TrampQueryPerformanceCounter
TRAMPFUNC BOOL WINAPI QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount);
#define QueryPerformanceFrequency TrampQueryPerformanceFrequency
TRAMPFUNC BOOL WINAPI QueryPerformanceFrequency(LARGE_INTEGER* lpPerformanceFrequency);
#define NtQueryPerformanceCounter TrampNtQueryPerformanceCounter
TRAMPFUNC NTSTATUS NTAPI NtQueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount, LARGE_INTEGER* lpPerformanceFrequency);

// these are timing-related but not really time-related,
// so it's probably not necessary to hook them.
//#define timeEndPeriod TramptimeEndPeriod
//TRAMPFUNC MMRESULT WINAPI timeEndPeriod(DWORD res);
//#define timeBeginPeriod TramptimeBeginPeriod
//TRAMPFUNC MMRESULT WINAPI timeBeginPeriod(DWORD res);
//#define timeGetDevCaps TramptimeGetDevCaps
//TRAMPFUNC MMRESULT WINAPI timeGetDevCaps(LPTIMECAPS ptc, UINT cbtc);

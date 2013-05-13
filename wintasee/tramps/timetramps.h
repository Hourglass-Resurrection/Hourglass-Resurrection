/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef TIMETRAMPS_H_INCL
#define TIMETRAMPS_H_INCL

#define timeGetTime TramptimeGetTime
TRAMPFUNC DWORD WINAPI timeGetTime(void) TRAMPOLINE_DEF
#define GetTickCount TrampGetTickCount
TRAMPFUNC DWORD WINAPI GetTickCount(void) TRAMPOLINE_DEF
#define GetSystemTimes TrampGetSystemTimes
TRAMPFUNC BOOL WINAPI GetSystemTimes(LPFILETIME lpIdleTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime) TRAMPOLINE_DEF
#define timeGetSystemTime TramptimeGetSystemTime
TRAMPFUNC MMRESULT WINAPI timeGetSystemTime(LPMMTIME pmmt, UINT cbmmt) TRAMPOLINE_DEF
#define GetSystemTime TrampGetSystemTime
TRAMPFUNC VOID WINAPI GetSystemTime(LPSYSTEMTIME lpSystemTime) TRAMPOLINE_DEF_VOID
#define GetSystemTimeAsFileTime TrampGetSystemTimeAsFileTime
TRAMPFUNC VOID WINAPI GetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime) TRAMPOLINE_DEF_VOID
#define GetLocalTime TrampGetLocalTime
TRAMPFUNC VOID WINAPI GetLocalTime(LPSYSTEMTIME lpSystemTime) TRAMPOLINE_DEF_VOID
#define NtQuerySystemTime TrampNtQuerySystemTime
TRAMPFUNC NTSTATUS NTAPI NtQuerySystemTime(PLARGE_INTEGER SystemTime) TRAMPOLINE_DEF
#define QueryPerformanceCounter TrampQueryPerformanceCounter
TRAMPFUNC BOOL WINAPI QueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount) TRAMPOLINE_DEF
#define QueryPerformanceFrequency TrampQueryPerformanceFrequency
TRAMPFUNC BOOL WINAPI QueryPerformanceFrequency(LARGE_INTEGER* lpPerformanceFrequency) TRAMPOLINE_DEF
#define NtQueryPerformanceCounter TrampNtQueryPerformanceCounter
TRAMPFUNC NTSTATUS NTAPI NtQueryPerformanceCounter(LARGE_INTEGER* lpPerformanceCount, LARGE_INTEGER* lpPerformanceFrequency) TRAMPOLINE_DEF

// these are timing-related but not really time-related,
// so it's probably not necessary to hook them.
//#define timeEndPeriod TramptimeEndPeriod
//TRAMPFUNC MMRESULT WINAPI timeEndPeriod(DWORD res) TRAMPOLINE_DEF
//#define timeBeginPeriod TramptimeBeginPeriod
//TRAMPFUNC MMRESULT WINAPI timeBeginPeriod(DWORD res) TRAMPOLINE_DEF
//#define timeGetDevCaps TramptimeGetDevCaps
//TRAMPFUNC MMRESULT WINAPI timeGetDevCaps(LPTIMECAPS ptc, UINT cbtc) TRAMPOLINE_DEF


#endif

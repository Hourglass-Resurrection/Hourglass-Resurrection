/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#include "../intercept.h"

namespace Hooks
{
    HOOK_DECLARE(DWORD, WINAPI, timeGetTime);
    HOOK_DECLARE(DWORD, WINAPI, GetTickCount);
    HOOK_DECLARE(BOOL, WINAPI, GetSystemTimes, LPFILETIME lpIdleTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime);
    HOOK_DECLARE(MMRESULT, WINAPI, timeGetSystemTime, LPMMTIME pmmt, UINT cbmmt);
    HOOK_DECLARE(VOID, WINAPI, GetSystemTime, LPSYSTEMTIME lpSystemTime);
    HOOK_DECLARE(VOID, WINAPI, GetSystemTimeAsFileTime, LPFILETIME lpSystemTimeAsFileTime);
    HOOK_DECLARE(VOID, WINAPI, GetLocalTime, LPSYSTEMTIME lpSystemTime);
    HOOK_DECLARE(NTSTATUS, NTAPI, NtQuerySystemTime, PLARGE_INTEGER SystemTime);
    HOOK_DECLARE(BOOL, WINAPI, QueryPerformanceCounter, LARGE_INTEGER* lpPerformanceCount);
    HOOK_DECLARE(BOOL, WINAPI, QueryPerformanceFrequency, LARGE_INTEGER* lpPerformanceFrequency);
    HOOK_DECLARE(NTSTATUS, NTAPI, NtQueryPerformanceCounter, LARGE_INTEGER* lpPerformanceCount, LARGE_INTEGER* lpPerformanceFrequency);

    // these are timing-related but not really time-related,
    // so it's probably not necessary to hook them.
    //HOOK_DECLARE(MMRESULT WINAPI timeEndPeriod(DWORD res);
    //HOOK_DECLARE(MMRESULT WINAPI timeBeginPeriod(DWORD res);
    //HOOK_DECLARE(MMRESULT WINAPI timeGetDevCaps(LPTIMECAPS ptc, UINT cbtc);

    int getCurrentThreadstamp();
    int getCurrentFramestamp();
    int getCurrentTimestamp();
    int getCurrentTimestamp2();
    int getCurrentTimestamp3();

    void ApplyTimeIntercepts();

    bool HookCOMInterfaceTime(REFIID riid, LPVOID* ppvOut, bool uncheckedFastNew);
}

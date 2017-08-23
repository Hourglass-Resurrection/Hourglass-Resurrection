/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define _WIN32_WINNT 0x0601
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <chrono>

#include "timing.h"

namespace Timing
{
    namespace
    {
        void(__stdcall *GetSystemTimePreciseAsFileTime)(LPFILETIME lpSystemTimeAsFileTime);
    }

    long long CurrentUSChronoSystem()
    {
        auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    }

    long long CurrentUSChronoSteady()
    {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    }

    long long CurrentUSChronoHighResolution()
    {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    }

    long long CurrentUSQueryPerformanceCounter()
    {
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);

        LARGE_INTEGER counter;
        QueryPerformanceCounter(&counter);

        return (counter.QuadPart * 1'000'000) / frequency.QuadPart;
    }

    long long CurrentUSGetSystemTimeAsFileTime()
    {
        FILETIME filetime;
        GetSystemTimeAsFileTime(&filetime);

        ULARGE_INTEGER large;
        large.HighPart = filetime.dwHighDateTime;
        large.LowPart = filetime.dwLowDateTime;

        // large.QuadPart is in 100s of nanoseconds.
        return large.QuadPart / 10;
    }

    long long CurrentUSGetSystemTimePreciseAsFileTime()
    {
        FILETIME filetime;
        GetSystemTimePreciseAsFileTime(&filetime);

        ULARGE_INTEGER large;
        large.HighPart = filetime.dwHighDateTime;
        large.LowPart = filetime.dwLowDateTime;

        // large.QuadPart is in 100s of nanoseconds.
        return large.QuadPart / 10;
    }

    void Initialize()
    {
        HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
        GetSystemTimePreciseAsFileTime = reinterpret_cast<void(__stdcall *)(LPFILETIME)>(
            GetProcAddress(kernel32, "GetSystemTimePreciseAsFileTime"));

        /*
         * Fallback for Windows 7 where this function did not exist yet.
         */
        if (!GetSystemTimePreciseAsFileTime)
            GetSystemTimePreciseAsFileTime = GetSystemTimeAsFileTime;
    }
}

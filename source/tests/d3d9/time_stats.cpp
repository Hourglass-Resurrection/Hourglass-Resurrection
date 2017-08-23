/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include "stdafx.h"
#include "logger.h"
#include "time_stats.h"

namespace TimeStats
{
    namespace
    {
        unsigned int gs_frame_counter = 0;

        long long gs_last_us_query_performance_counter;

        long long CurrentUSQueryPerformanceCounter()
        {
            LARGE_INTEGER frequency;
            QueryPerformanceFrequency(&frequency);

            LARGE_INTEGER counter;
            QueryPerformanceCounter(&counter);

            return (counter.QuadPart * 1'000'000) / frequency.QuadPart;
        }
    }

    void PreRender()
    {
        ++gs_frame_counter;

        Logger::WriteLine(L"Frame " + std::to_wstring(gs_frame_counter) + L" pre render:");

        long long current_us_query_performance_counter = CurrentUSQueryPerformanceCounter();
        Logger::Write(L"    QueryPerformanceCounter: " + std::to_wstring(current_us_query_performance_counter) + L"us");

        if (gs_frame_counter > 1)
            Logger::Write(L" (frametime: " + std::to_wstring(current_us_query_performance_counter - gs_last_us_query_performance_counter) + L"us)");

        Logger::WriteLine(L"");

        gs_last_us_query_performance_counter = current_us_query_performance_counter;
    }

    void PostRender()
    {
        Logger::WriteLine(L"Frame " + std::to_wstring(gs_frame_counter) + L" post render:");
        Logger::WriteLine(L"    QueryPerformanceCounter: " + std::to_wstring(CurrentUSQueryPerformanceCounter()) + L"us");
        Logger::WriteLine(L"");
    }
}

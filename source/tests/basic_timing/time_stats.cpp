/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include "stdafx.h"
#include "shared/logger.h"
#include "shared/timing.h"
#include "time_stats.h"

namespace TimeStats
{
    namespace
    {
        unsigned int gs_frame_counter = 0;

        long long gs_last_us_chrono_system;
        long long gs_last_us_chrono_steady;
        long long gs_last_us_chrono_high_resolution;
        long long gs_last_us_query_performance_counter;
        long long gs_last_us_get_system_time;
        long long gs_last_us_get_system_time_precise;
    }

    void PreRender()
    {
        ++gs_frame_counter;

        Logger::WriteLine(L"Frame " + std::to_wstring(gs_frame_counter) + L" pre render:");

        {
            long long current_us_chrono_system = Timing::CurrentUSChronoSystem();
            Logger::Write(L"    std::chrono system_clock: " + std::to_wstring(current_us_chrono_system) + L"us");

            if (gs_frame_counter > 1)
                Logger::Write(L" (frametime: " + std::to_wstring(current_us_chrono_system - gs_last_us_chrono_system) + L"us)");

            Logger::WriteLine(L"");

            gs_last_us_chrono_system = current_us_chrono_system;
        }

        {
            long long current_us_chrono_steady = Timing::CurrentUSChronoSteady();
            Logger::Write(L"    std::chrono steady_clock: " + std::to_wstring(current_us_chrono_steady) + L"us");

            if (gs_frame_counter > 1)
                Logger::Write(L" (frametime: " + std::to_wstring(current_us_chrono_steady - gs_last_us_chrono_steady) + L"us)");

            Logger::WriteLine(L"");

            gs_last_us_chrono_steady = current_us_chrono_steady;
        }

        {
            long long current_us_chrono_high_resolution = Timing::CurrentUSChronoHighResolution();
            Logger::Write(L"    std::chrono high_resolution_clock: " + std::to_wstring(current_us_chrono_high_resolution) + L"us");

            if (gs_frame_counter > 1)
                Logger::Write(L" (frametime: " + std::to_wstring(current_us_chrono_high_resolution - gs_last_us_chrono_high_resolution) + L"us)");

            Logger::WriteLine(L"");

            gs_last_us_chrono_high_resolution = current_us_chrono_high_resolution;
        }

        {
            long long current_us_query_performance_counter = Timing::CurrentUSQueryPerformanceCounter();
            Logger::Write(L"    QueryPerformanceCounter: " + std::to_wstring(current_us_query_performance_counter) + L"us");

            if (gs_frame_counter > 1)
                Logger::Write(L" (frametime: " + std::to_wstring(current_us_query_performance_counter - gs_last_us_query_performance_counter) + L"us)");

            Logger::WriteLine(L"");

            gs_last_us_query_performance_counter = current_us_query_performance_counter;
        }

        {
            long long current_us_get_system_time = Timing::CurrentUSGetSystemTimeAsFileTime();
            Logger::Write(L"    GetSystemTimeAsFileTime: " + std::to_wstring(current_us_get_system_time) + L"us");

            if (gs_frame_counter > 1)
                Logger::Write(L" (frametime: " + std::to_wstring(current_us_get_system_time - gs_last_us_get_system_time) + L"us)");

            Logger::WriteLine(L"");

            gs_last_us_get_system_time = current_us_get_system_time;
        }

        {
            long long current_us_get_system_time_precise = Timing::CurrentUSGetSystemTimePreciseAsFileTime();
            Logger::Write(L"    GetSystemTimePreciseAsFileTime: " + std::to_wstring(current_us_get_system_time_precise) + L"us");

            if (gs_frame_counter > 1)
                Logger::Write(L" (frametime: " + std::to_wstring(current_us_get_system_time_precise - gs_last_us_get_system_time_precise) + L"us)");

            Logger::WriteLine(L"");

            gs_last_us_get_system_time_precise = current_us_get_system_time_precise;
        }
    }

    void PostRender()
    {
        Logger::WriteLine(L"Frame " + std::to_wstring(gs_frame_counter) + L" post render:");
        Logger::WriteLine(L"    std::chrono system_clock: " + std::to_wstring(Timing::CurrentUSChronoSystem()) + L"us");
        Logger::WriteLine(L"    std::chrono steady_clock: " + std::to_wstring(Timing::CurrentUSChronoSteady()) + L"us");
        Logger::WriteLine(L"    std::chrono high_resolution_clock: " + std::to_wstring(Timing::CurrentUSChronoHighResolution()) + L"us");
        Logger::WriteLine(L"    QueryPerformanceCounter: " + std::to_wstring(Timing::CurrentUSQueryPerformanceCounter()) + L"us");
        Logger::WriteLine(L"    GetSystemTimeAsFileTime: " + std::to_wstring(Timing::CurrentUSGetSystemTimeAsFileTime()) + L"us");
        Logger::WriteLine(L"    GetSystemTimePreciseAsFileTime: " + std::to_wstring(Timing::CurrentUSGetSystemTimePreciseAsFileTime()) + L"us");
        Logger::WriteLine(L"");
    }
}

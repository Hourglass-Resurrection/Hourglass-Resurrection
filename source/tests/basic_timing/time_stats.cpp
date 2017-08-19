#include "stdafx.h"
#include "logger.h"
#include "time_stats.h"

namespace TimeStats
{
    namespace
    {
        unsigned gs_frame_counter = 0;

        long long gs_last_us_chrono_system;
        long long gs_last_us_chrono_steady;
        long long gs_last_us_chrono_high_resolution;
        long long gs_last_us_query_performance_counter;
        long long gs_last_us_get_system_time;

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
    }

    void PreRender()
    {
        ++gs_frame_counter;

        Logger::WriteLine(L"Frame " + std::to_wstring(gs_frame_counter) + L" pre render:");

        {
            long long current_us_chrono_system = CurrentUSChronoSystem();
            Logger::Write(L"    std::chrono system_clock: " + std::to_wstring(current_us_chrono_system) + L"us");

            if (gs_frame_counter > 1)
                Logger::Write(L" (frametime: " + std::to_wstring(current_us_chrono_system - gs_last_us_chrono_system) + L"us)");

            Logger::WriteLine(L"");

            gs_last_us_chrono_system = current_us_chrono_system;
        }

        {
            long long current_us_chrono_steady = CurrentUSChronoSteady();
            Logger::Write(L"    std::chrono steady_clock: " + std::to_wstring(current_us_chrono_steady) + L"us");

            if (gs_frame_counter > 1)
                Logger::Write(L" (frametime: " + std::to_wstring(current_us_chrono_steady - gs_last_us_chrono_steady) + L"us)");

            Logger::WriteLine(L"");

            gs_last_us_chrono_steady = current_us_chrono_steady;
        }

        {
            long long current_us_chrono_high_resolution = CurrentUSChronoHighResolution();
            Logger::Write(L"    std::chrono high_resolution_clock: " + std::to_wstring(current_us_chrono_high_resolution) + L"us");

            if (gs_frame_counter > 1)
                Logger::Write(L" (frametime: " + std::to_wstring(current_us_chrono_high_resolution - gs_last_us_chrono_high_resolution) + L"us)");

            Logger::WriteLine(L"");

            gs_last_us_chrono_high_resolution = current_us_chrono_high_resolution;
        }

        {
            long long current_us_query_performance_counter = CurrentUSQueryPerformanceCounter();
            Logger::Write(L"    QueryPerformanceCounter: " + std::to_wstring(current_us_query_performance_counter) + L"us");

            if (gs_frame_counter > 1)
                Logger::Write(L" (frametime: " + std::to_wstring(current_us_query_performance_counter - gs_last_us_query_performance_counter) + L"us)");

            Logger::WriteLine(L"");

            gs_last_us_query_performance_counter = current_us_query_performance_counter;
        }

        {
            long long current_us_get_system_time = CurrentUSGetSystemTimeAsFileTime();
            Logger::Write(L"    GetSystemTimeAsFileTime: " + std::to_wstring(current_us_get_system_time) + L"us");

            if (gs_frame_counter > 1)
                Logger::Write(L" (frametime: " + std::to_wstring(current_us_get_system_time - gs_last_us_get_system_time) + L"us)");

            Logger::WriteLine(L"");

            gs_last_us_get_system_time = current_us_get_system_time;
        }
    }

    void PostRender()
    {
        Logger::WriteLine(L"Frame " + std::to_wstring(gs_frame_counter) + L" post render:");
        Logger::WriteLine(L"    std::chrono system_clock: " + std::to_wstring(CurrentUSChronoSystem()) + L"us");
        Logger::WriteLine(L"    std::chrono steady_clock: " + std::to_wstring(CurrentUSChronoSteady()) + L"us");
        Logger::WriteLine(L"    std::chrono high_resolution_clock: " + std::to_wstring(CurrentUSChronoHighResolution()) + L"us");
        Logger::WriteLine(L"    QueryPerformanceCounter: " + std::to_wstring(CurrentUSQueryPerformanceCounter()) + L"us");
        Logger::WriteLine(L"    GetSystemTimeAsFileTime: " + std::to_wstring(CurrentUSGetSystemTimeAsFileTime()) + L"us");
        Logger::WriteLine(L"");
    }
}

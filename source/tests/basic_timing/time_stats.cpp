#include "stdafx.h"
#include "logger.h"
#include "time_stats.h"

namespace TimeStats
{
    namespace
    {
        unsigned gs_frame_counter = 0;

        long long gs_last_ns_chrono_system;
        long long gs_last_ns_chrono_steady;
        long long gs_last_ns_chrono_high_resolution;
        long long gs_last_ns_query_performance_counter;

        long long CurrentNSChronoSystem()
        {
            auto now = std::chrono::system_clock::now();
            return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        }

        long long CurrentNSChronoSteady()
        {
            auto now = std::chrono::steady_clock::now();
            return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        }

        long long CurrentNSChronoHighResolution()
        {
            auto now = std::chrono::high_resolution_clock::now();
            return std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        }

        long long CurrentNSQueryPerformanceCounter()
        {
            LARGE_INTEGER frequency;
            QueryPerformanceFrequency(&frequency);

            LARGE_INTEGER counter;
            QueryPerformanceCounter(&counter);

            return (counter.QuadPart * 1'000'000'000) / frequency.QuadPart;
        }
    }

    void PreRender()
    {
        ++gs_frame_counter;

        Logger::WriteLine(L"Frame " + std::to_wstring(gs_frame_counter) + L" pre render:");

        {
            long long current_ns_chrono_system = CurrentNSChronoSystem();
            Logger::Write(L"    std::chrono system_clock: " + std::to_wstring(current_ns_chrono_system) + L"ns");

            if (gs_frame_counter > 1)
                Logger::Write(L" (frametime: " + std::to_wstring(current_ns_chrono_system - gs_last_ns_chrono_system) + L"ns)");

            Logger::WriteLine(L"");

            gs_last_ns_chrono_system = current_ns_chrono_system;
        }

        {
            long long current_ns_chrono_steady = CurrentNSChronoSteady();
            Logger::Write(L"    std::chrono steady_clock: " + std::to_wstring(current_ns_chrono_steady) + L"ns");

            if (gs_frame_counter > 1)
                Logger::Write(L" (frametime: " + std::to_wstring(current_ns_chrono_steady - gs_last_ns_chrono_steady) + L"ns)");

            Logger::WriteLine(L"");

            gs_last_ns_chrono_steady = current_ns_chrono_steady;
        }

        {
            long long current_ns_chrono_high_resolution = CurrentNSChronoHighResolution();
            Logger::Write(L"    std::chrono high_resolution_clock: " + std::to_wstring(current_ns_chrono_high_resolution) + L"ns");

            if (gs_frame_counter > 1)
                Logger::Write(L" (frametime: " + std::to_wstring(current_ns_chrono_high_resolution - gs_last_ns_chrono_high_resolution) + L"ns)");

            Logger::WriteLine(L"");

            gs_last_ns_chrono_high_resolution = current_ns_chrono_high_resolution;
        }

        {
            long long current_ns_query_performance_counter = CurrentNSQueryPerformanceCounter();
            Logger::Write(L"    QueryPerformanceCounter: " + std::to_wstring(current_ns_query_performance_counter) + L"ns");

            if (gs_frame_counter > 1)
                Logger::Write(L" (frametime: " + std::to_wstring(current_ns_query_performance_counter - gs_last_ns_query_performance_counter) + L"ns)");

            Logger::WriteLine(L"");

            gs_last_ns_query_performance_counter = current_ns_query_performance_counter;
        }
    }

    void PostRender()
    {
        Logger::WriteLine(L"Frame " + std::to_wstring(gs_frame_counter) + L" post render:");
        Logger::WriteLine(L"    std::chrono system_clock: " + std::to_wstring(CurrentNSChronoSystem()) + L"ns");
        Logger::WriteLine(L"    std::chrono steady_clock: " + std::to_wstring(CurrentNSChronoSteady()) + L"ns");
        Logger::WriteLine(L"    std::chrono high_resolution_clock: " + std::to_wstring(CurrentNSChronoHighResolution()) + L"ns");
        Logger::WriteLine(L"    QueryPerformanceCounter: " + std::to_wstring(CurrentNSQueryPerformanceCounter()) + L"ns");
        Logger::WriteLine(L"");
    }
}

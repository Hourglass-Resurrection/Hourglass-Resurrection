/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

namespace Timing
{
    void Initialize();

    long long CurrentUSChronoSystem();
    long long CurrentUSChronoSteady();
    long long CurrentUSChronoHighResolution();
    long long CurrentUSQueryPerformanceCounter();
    long long CurrentUSGetSystemTimeAsFileTime();
    long long CurrentUSGetSystemTimePreciseAsFileTime();
}

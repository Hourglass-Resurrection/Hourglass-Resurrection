/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace Utils
{
    namespace Thread
    {
        HANDLE CreateThread(LPSECURITY_ATTRIBUTES thread_attributes,
                            SIZE_T stack_size,
                            LPTHREAD_START_ROUTINE start_address,
                            LPVOID parameter,
                            DWORD creation_flags,
                            LPDWORD thread_id);
    }
}

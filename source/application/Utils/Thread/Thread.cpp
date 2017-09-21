/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "../Exceptions.h"

#include "../Thread.h"

namespace
{
    struct ThreadParams
    {
        LPTHREAD_START_ROUTINE function;
        LPVOID param;
    };

    static DWORD WINAPI PreThreadProc(LPVOID param)
    {
        /*
         * Init code for every thread.
         */
        Utils::Exceptions::InitWindowsExceptionsHandler();

        /*
         * Run actual thread proc.
         */
        ThreadParams* thread_params = reinterpret_cast<ThreadParams*>(param);
        DWORD ret_code = thread_params->function(thread_params->param);
        delete thread_params;
        return ret_code;
    }
}

HANDLE Utils::Thread::CreateThread(LPSECURITY_ATTRIBUTES thread_attributes,
                                   SIZE_T stack_size,
                                   LPTHREAD_START_ROUTINE start_address,
                                   LPVOID parameter,
                                   DWORD creation_flags,
                                   LPDWORD thread_id)
{
    /*
     * Deleted by the PreThreadProc.
     */
    ThreadParams* thread_params = new ThreadParams();
    thread_params->function = start_address;
    thread_params->param = parameter;
    return ::CreateThread(thread_attributes,
                          stack_size,
                          PreThreadProc,
                          thread_params,
                          creation_flags,
                          thread_id);
}

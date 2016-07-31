/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#include "../intercept.h"

namespace Hooks
{
    HOOK_FUNCTION_DECLARE(HANDLE, WINAPI, CreateThread,
        LPSECURITY_ATTRIBUTES lpThreadAttributes,
        SIZE_T dwStackSize,
        LPTHREAD_START_ROUTINE lpStartAddress,
        LPVOID lpParameter,
        DWORD dwCreationFlags,
        LPDWORD lpThreadId
    );
    HOOK_FUNCTION_DECLARE(VOID, WINAPI, ExitThread, DWORD dwExitCode);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, TerminateThread, HANDLE hThread, DWORD dwExitCode);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, GetExitCodeThread, HANDLE hThread, LPDWORD lpExitCode);
    HOOK_FUNCTION_DECLARE(NTSTATUS, NTAPI, NtSetInformationThread, HANDLE ThreadHandle, DWORD ThreadInformationClass, PVOID ThreadInformation, ULONG ThreadInformationLength);

#ifdef TlsSetValue
    #error this shouldn't happen (TlsSetValue already defined)
#endif

    // hack: because we need to call TlsSetValue/TlsGetValue potentially very early in the startup process,
    // let the TlsSetValue/TlsGetValue macros act like trampolines both before and after their respective functions have been hooked.
    // this might seem universally safer, but the reason we usually avoid this way of defining trampolines is because:
    // if the function doesn't exist in the DLL, it will cause the game to immediately crash on startup.
    // for example, it's not safe to do this for FlsSetValue/FlsGetValue because those don't exist on Windows XP.
    //HOOK_FUNCTION_DECLARE(BOOL WINAPI TrampTlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue) { return TlsSetValue(dwTlsIndex, lpTlsValue); }
    //HOOK_FUNCTION_DECLARE(LPVOID WINAPI TrampTlsGetValue(DWORD dwTlsIndex) { return TlsGetValue(dwTlsIndex); }
    //
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, FlsSetValue, DWORD dwFlsIndex, LPVOID lpFlsData);
    HOOK_FUNCTION_DECLARE(PVOID, WINAPI, FlsGetValue, DWORD dwFlsIndex);

    void SetThreadName(DWORD dwThreadID, char* threadName);

    void ApplyThreadIntercepts();
}

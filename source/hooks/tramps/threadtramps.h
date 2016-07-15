/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#include "../intercept.h"

namespace Hooks
{
    HOOK_DECLARE(HANDLE, WINAPI, CreateThread,
        LPSECURITY_ATTRIBUTES lpThreadAttributes,
        SIZE_T dwStackSize,
        LPTHREAD_START_ROUTINE lpStartAddress,
        LPVOID lpParameter,
        DWORD dwCreationFlags,
        LPDWORD lpThreadId
    );
    HOOK_DECLARE(VOID, WINAPI, ExitThread, DWORD dwExitCode);
    HOOK_DECLARE(BOOL, WINAPI, TerminateThread, HANDLE hThread, DWORD dwExitCode);
    HOOK_DECLARE(BOOL, WINAPI, GetExitCodeThread, HANDLE hThread, LPDWORD lpExitCode);
    HOOK_DECLARE(NTSTATUS, NTAPI, NtSetInformationThread, HANDLE ThreadHandle, DWORD ThreadInformationClass, PVOID ThreadInformation, ULONG ThreadInformationLength);

    void SetThreadName(DWORD dwThreadID, char* threadName);

    void ApplyThreadIntercepts();
}

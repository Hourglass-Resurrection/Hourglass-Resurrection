/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#include "../global.h"
#include "shared/ipc.h"
#include "../print.h"

using Log = DebugLog<LogCategory::FILEIO>;

namespace Hooks
{
    HOOK_FUNCTION(HANDLE, WINAPI, CreateFileA,
        LPCSTR lpFileName,
        DWORD dwDesiredAccess,
        DWORD dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        DWORD dwCreationDisposition,
        DWORD dwFlagsAndAttributes,
        HANDLE hTemplateFile
    );
    HOOKFUNC HANDLE WINAPI MyCreateFileA(LPCSTR lpFileName,
        DWORD dwDesiredAccess,
        DWORD dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        DWORD dwCreationDisposition,
        DWORD dwFlagsAndAttributes,
        HANDLE hTemplateFile
    )
    {
        ENTER(dwDesiredAccess, lpFileName);
        return CreateFileA(
            lpFileName,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            hTemplateFile
        );
    }

    HOOK_FUNCTION(HANDLE, WINAPI, CreateFileW,
        LPCWSTR lpFileName,
        DWORD dwDesiredAccess,
        DWORD dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        DWORD dwCreationDisposition,
        DWORD dwFlagsAndAttributes,
        HANDLE hTemplateFile
    );
    HOOKFUNC HANDLE WINAPI MyCreateFileW(LPCWSTR lpFileName,
        DWORD dwDesiredAccess,
        DWORD dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        DWORD dwCreationDisposition,
        DWORD dwFlagsAndAttributes,
        HANDLE hTemplateFile
    )
    {
        ENTER(dwDesiredAccess, lpFileName);
        return CreateFileW(
            lpFileName,
            dwDesiredAccess,
            dwShareMode,
            lpSecurityAttributes,
            dwCreationDisposition,
            dwFlagsAndAttributes,
            hTemplateFile
        );
    }

    HOOK_FUNCTION(HFILE, WINAPI, OpenFile,
        LPCSTR lpFileName, LPOFSTRUCT lpReOpenBuff, UINT uStyle);
    HOOKFUNC HFILE WINAPI MyOpenFile(LPCSTR lpFileName, LPOFSTRUCT lpReOpenBuff, UINT uStyle)
    {
        ENTER(uStyle, lpFileName);
        return OpenFile(lpFileName, lpReOpenBuff, uStyle);
    }

    void ApplyFileIntercepts()
    {
        static const InterceptDescriptor intercepts[] =
        {
        MAKE_INTERCEPT(1, KERNEL32, CreateFileA),
        MAKE_INTERCEPT(1, KERNEL32, CreateFileW),
        MAKE_INTERCEPT(1, KERNEL32, OpenFile),
        };
        ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
    }
}

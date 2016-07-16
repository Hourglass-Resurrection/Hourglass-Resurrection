/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#include "../intercept.h"

namespace Hooks
{
    HOOK_FUNCTION_DECLARE(HANDLE, WINAPI, CreateFileA,
        LPCSTR lpFileName,
        DWORD dwDesiredAccess,
        DWORD dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        DWORD dwCreationDisposition,
        DWORD dwFlagsAndAttributes,
        HANDLE hTemplateFile
    );

    HOOK_FUNCTION_DECLARE(HANDLE, WINAPI, CreateFileW,
        LPCWSTR lpFileName,
        DWORD dwDesiredAccess,
        DWORD dwShareMode,
        LPSECURITY_ATTRIBUTES lpSecurityAttributes,
        DWORD dwCreationDisposition,
        DWORD dwFlagsAndAttributes,
        HANDLE hTemplateFile
    );

    HOOK_FUNCTION_DECLARE(HFILE, WINAPI, OpenFile, LPCSTR lpFileName, LPOFSTRUCT lpReOpenBuff, UINT uStyle);

    void ApplyFileIntercepts();
}

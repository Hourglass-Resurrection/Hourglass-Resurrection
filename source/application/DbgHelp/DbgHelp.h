/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <functional>

#include "IDbgHelpStackWalkCallback.h"

namespace DbgHelp
{
    void Init();

    void AddProcess(HANDLE process, DWORD process_id);
    void RemoveProcess(DWORD process_id);

    void LoadSymbols(DWORD process_id, HANDLE module_file, LPCWSTR module_name, DWORD64 module_base);

    using StackWalkCallback = std::function<IDbgHelpStackWalkCallback::Action(IDbgHelpStackWalkCallback&)>;
    void StackWalk(DWORD process_id, HANDLE thread, StackWalkCallback cb);
};

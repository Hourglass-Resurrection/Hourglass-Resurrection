/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "DbgHelpStackWalkCallback.h"

class DbgHelpPrivate;

class DbgHelp
{
public:
    DbgHelp();
    ~DbgHelp();

    void AddProcess(HANDLE process, DWORD process_id);
    void LoadSymbols(DWORD process_id, HANDLE module_file, LPCWSTR module_name, DWORD64 module_base);

    using StackWalkCallback = std::function<DbgHelpStackWalkCallback::Action(const DbgHelpStackWalkCallback&)>;
    void StackWalk(DWORD process_id, HANDLE thread, StackWalkCallback cb);

private:
    std::map<DWORD, std::unique_ptr<DbgHelpPrivate>> m_private_map;
    std::wstring m_symbol_paths;
};

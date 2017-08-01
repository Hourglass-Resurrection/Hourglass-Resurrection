/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <array>
#include <memory>
#include <vector>

#include "DbgHelpPrivate.h"

#include "DbgHelp.h"

namespace
{
    std::map<DWORD, std::unique_ptr<DbgHelpPrivate>> s_private_map;
    std::wstring s_symbol_paths;
}

namespace DbgHelp
{
    void Init()
    {
        std::array<WCHAR, 0x1000> buffer;
        buffer.fill('\0');
        s_symbol_paths.append(L".");

        if (GetCurrentDirectoryW(buffer.size(), buffer.data()) != 0)
        {
            s_symbol_paths.append(L";").append(buffer.data());
        }
        if (GetEnvironmentVariableW(L"_NT_SYMBOL_PATH", buffer.data(), buffer.size()) != 0)
        {
            s_symbol_paths.append(L";").append(buffer.data());
        }
        if (GetEnvironmentVariableW(L"_NT_ALTERNATIVE_SYMBOL_PATH", buffer.data(), buffer.size()) != 0)
        {
            s_symbol_paths.append(L";").append(buffer.data());
        }
        if (GetEnvironmentVariableW(L"SYSTEMROOT", buffer.data(), buffer.size()) != 0)
        {
            s_symbol_paths.append(L";").append(buffer.data())
                .append(L";").append(buffer.data()).append(L"\\system32")
                .append(L";").append(buffer.data()).append(L"\\SysWOW64");
        }
        /*
         * TODO: Enable when SymbolServer support is added. Needs symsrv.dll.
         * This will enable downloading of symbols for the system DLLs making stack traces more accurate.
         * -- Warepire
         */
         //symbol_paths.append(";").append("SRV*%SYSTEMDRIVE%\\websymbols*http://msdl.microsoft.com/download/symbols");
    }

    void AddProcess(HANDLE process, DWORD process_id)
    {
        s_private_map.emplace(process_id, std::make_unique<DbgHelpPrivate>(process));
    }

    void RemoveProcess(DWORD process_id)
    {
        s_private_map.erase(process_id);
    }

    void LoadSymbols(DWORD process_id, HANDLE module_file, LPCWSTR module_name, DWORD64 module_base)
    {
        s_private_map.at(process_id)->LoadSymbols(module_base, module_name, s_symbol_paths);
    }

    void StackWalk(DWORD process_id, HANDLE thread, DbgHelp::StackWalkCallback cb)
    {
        s_private_map.at(process_id)->StackWalk(thread, cb);
    }
}
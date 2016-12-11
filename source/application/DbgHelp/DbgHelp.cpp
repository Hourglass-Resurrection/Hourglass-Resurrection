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

DbgHelp::DbgHelp()
{
    std::array<WCHAR, 0x1000> buffer;
    buffer.fill('\0');
    m_symbol_paths.append(L".");

    if (GetCurrentDirectoryW(buffer.size(), buffer.data()) != 0)
    {
        m_symbol_paths.append(L";").append(buffer.data());
    }
    if (GetEnvironmentVariableW(L"_NT_SYMBOL_PATH", buffer.data(), buffer.size()) != 0)
    {
        m_symbol_paths.append(L";").append(buffer.data());
    }
    if (GetEnvironmentVariableW(L"_NT_ALTERNATIVE_SYMBOL_PATH", buffer.data(), buffer.size()) != 0)
    {
        m_symbol_paths.append(L";").append(buffer.data());
    }
    if (GetEnvironmentVariableW(L"SYSTEMROOT", buffer.data(), buffer.size()) != 0)
    {
        m_symbol_paths.append(L";").append(buffer.data())
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

DbgHelp::~DbgHelp()
{
    m_private_map.clear();
}

void DbgHelp::AddProcess(HANDLE process, DWORD process_id)
{
    m_private_map.emplace(process_id, std::make_unique<DbgHelpPrivate>(process));
}

void DbgHelp::LoadSymbols(DWORD process_id, HANDLE module_file, LPCWSTR module_name, DWORD64 module_base)
{
    m_private_map.at(process_id)->LoadSymbols(module_base, module_name, m_symbol_paths);
}

void DbgHelp::StackWalk(DWORD process_id, HANDLE thread, DbgHelp::StackWalkCallback cb)
{
    m_private_map.at(process_id)->StackWalk(thread, cb);
}

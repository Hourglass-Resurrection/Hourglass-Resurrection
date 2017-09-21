/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <DIA SDK/include/dia2.h>
#include <atlbase.h>
#include <atlcom.h>

#include "DbgHelp.h"
#include "application/Utils/COM.h"

class DbgHelpPrivate
{
public:
    struct ModuleData
    {
        DWORD m_module_size;
        std::wstring m_module_name;
        CComPtr<IDiaSession> m_module_symbol_session;
        std::map<DWORD64, std::wstring> m_module_exports_table;
    };

    DbgHelpPrivate(const Utils::COM::COMLibrary& dia, HANDLE process);
    ~DbgHelpPrivate();

    bool LoadSymbols(DWORD64 module_base,
                     const std::wstring& exec,
                     const std::wstring& search_path);
    bool StackWalk(HANDLE thread, DbgHelp::StackWalkCallback& cb);

    HANDLE GetProcess() const;
    const ModuleData* GetModuleData(ULONGLONG virtual_address) const;

private:
    const Utils::COM::COMLibrary& m_dia;

    HANDLE m_process;
    /*
     * It's not possible to safely get the full path of the loaded module once we're in a
     * stack walking scenario, but, since we know the full path while loading symbols we
     * can save the address and path of the module, and look it up during the stack trace.
     * -- Warepire
     */
    std::map<DWORD64, ModuleData> m_loaded_modules;

    /*
     * Assume everything is compiled for the same platform.
     */
    CV_CPU_TYPE_e m_platform;
    bool m_platform_set;
};

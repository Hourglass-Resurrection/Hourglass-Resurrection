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

class DbgHelpPriv;

class DbgHelp
{
public:
    struct StacktraceInfo
    {
        /*
         * Help constructor for faster addition of StacktraceInfo structs to the
         * member vector.
         */
        StacktraceInfo(DWORD64 address, const std::string& module_path) :
            m_address(address),
            m_module_path(module_path)
        {
        }
        DWORD64 m_address;
        std::string m_module_path;
    };

    explicit DbgHelp(HANDLE process);
    ~DbgHelp();

    void LoadSymbols(HANDLE module_file, LPCSTR module_name, DWORD64 module_base);
    std::vector<StacktraceInfo> Stacktrace(HANDLE thread, INT max_depth = -1);
    std::vector<std::wstring> GetFunctionTrace(const std::vector<StacktraceInfo>& trace);
    std::vector<std::wstring> GetFullTrace(const std::vector<StacktraceInfo>& trace);

private:
    /*
     * Basic symbol information getters
     */
    bool GetModuleBaseAndSymIndex(DWORD64 address, PDWORD64 mod_base, PULONG index);
    DWORD GetSymbolTag(DWORD64 module_base, ULONG type_index);
    DWORD GetCallingConvention(DWORD64 module_base, ULONG type_index);
    DWORD GetParamCount(DWORD64 module_base, ULONG type_index);
    DWORD GetParamCountForClass(DWORD64 module_base, ULONG type_index);
    DWORD GetParentClass(DWORD64 module_base, ULONG type_index);
    DWORD GetThisPtrOffset(DWORD64 module_base, ULONG type_index);
    std::wstring GetTypeName(DWORD64 module_base, ULONG type_index);
    std::wstring GetSymbolName(DWORD64 module_base, ULONG type_index);

    HANDLE m_process;

    /*
     * It's not possible to safely get the full path of the loaded module once we're in a
     * stack tracing scenario, but, since we know the full path while loading symbols we
     * can save the address and path of the module, and look it up during the stack trace.
     * -- Warepire
     */
    std::unique_ptr<DbgHelpPriv> m_private;
    std::wstring m_symbol_paths;
    std::map<DWORD64, std::string> m_loaded_modules;
};

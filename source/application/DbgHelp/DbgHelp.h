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

class DbgHelpPrivate;

class DbgHelp
{
public:
    struct FrameRegisterInfo
    {
        DWORD   Edi;
        DWORD   Esi;
        DWORD   Ebx;
        DWORD   Edx;
        DWORD   Ecx;
        DWORD   Eax;

        DWORD   Ebp;
        DWORD   Eip;
        DWORD   Esp;
    };
    struct StackFrameInfo
    {
        /*
         * Help constructor for faster addition of StacktraceInfo structs to the
         * member vector.
         */
        StackFrameInfo(DWORD64 address, const std::wstring& module_path) :
            m_address(address),
            m_module_path(module_path)
        {
        }
        DWORD64 m_address;
        std::wstring m_module_path;
        FrameRegisterInfo m_regs;
    };

    explicit DbgHelp(HANDLE process);
    ~DbgHelp();

    void LoadSymbols(HANDLE module_file, LPCWSTR module_name, DWORD64 module_base);
    std::vector<StackFrameInfo> Stacktrace(HANDLE thread, INT max_depth = -1);
    std::vector<std::wstring> GetFunctionTrace(const std::vector<StackFrameInfo>& trace);
    std::vector<std::wstring> GetFullTrace(const std::vector<StackFrameInfo>& trace);

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

    /*
     * It's not possible to safely get the full path of the loaded module once we're in a
     * stack tracing scenario, but, since we know the full path while loading symbols we
     * can save the address and path of the module, and look it up during the stack trace.
     * -- Warepire
     */
    std::unique_ptr<DbgHelpPrivate> m_private;
    std::wstring m_symbol_paths;
    std::map<DWORD64, std::wstring> m_loaded_modules;
};

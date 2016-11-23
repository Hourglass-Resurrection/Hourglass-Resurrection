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

/*
 * TODO: Break up into several files, create directory!
 * -- Warepire
 */

DbgHelp::DbgHelp(HANDLE process) :
    m_private(std::make_unique<DbgHelpPrivate>(process))
{
    std::array<WCHAR, 0x1000> buffer;
    buffer.fill('\0');

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

}

void DbgHelp::LoadSymbols(HANDLE module_file, LPCWSTR module_name, DWORD64 module_base)
{
    if (m_private->LoadSymbols(module_base, module_name, m_symbol_paths))
    {
        m_loaded_modules.emplace(module_base, module_name);
    }
}

std::vector<DbgHelp::StackFrameInfo> DbgHelp::Stacktrace(HANDLE thread, INT max_depth)
{
    std::vector<DbgHelp::StackFrameInfo> trace;
    //IDiaStackWalker* walker = nullptr;

    if (m_loaded_modules.empty())
    {
        return trace;
    }



    //stack_frame.AddrPC.Offset = thread_context.Eip;
    //stack_frame.AddrPC.Mode = AddrModeFlat;
    //stack_frame.AddrStack.Offset = thread_context.Esp;
    //stack_frame.AddrStack.Mode = AddrModeFlat;
    //stack_frame.AddrFrame.Offset = thread_context.Ebp;
    //stack_frame.AddrFrame.Mode = AddrModeFlat;

    //for (INT i = 0; ; i++)
    //{
    //    BOOL rv = StackWalk64Pointer(IMAGE_FILE_MACHINE_I386, m_process, thread,
    //                                 &stack_frame, &thread_context, nullptr,
    //                                 SymFunctionTableAccess64Pointer, SymGetModuleBase64Pointer,
    //                                 nullptr);
    //    if (rv == FALSE)
    //    {
    //        break;
    //    }
    //    DWORD64 address = stack_frame.AddrPC.Offset;

    //    /*
    //     * lower_bound() will return an iterator to the location in the map where 'address' should
    //     * be inserted, be it used with i.e. emplace_hint(). We can thus use it to look up the
    //     * module 'address' belongs to by getting this iterator, and then iterate backwards once.
    //     */
    //    auto module_it = m_loaded_modules.lower_bound(address);
    //    module_it--;
    //    trace.emplace_back(address, module_it->second);

    //    if (i >= max_depth)
    //    {
    //        break;
    //    }
    //}
    return trace;
}

//std::vector<std::wstring> DbgHelp::GetFunctionTrace(const std::vector<StacktraceInfo>& trace)
//{
//    std::vector<std::wstring> functions(trace.size());
//    DWORD64 mod_address;
//    ULONG type_index;
//
//    for (auto& i = trace.begin(); i != trace.end(); i++)
//    {
//        if (GetModuleBaseAndSymIndex(i->m_address, &mod_address, &type_index) &&
//            GetSymbolTag(mod_address, type_index) == SymTagFunction)
//        {
//            functions.emplace_back(GetSymbolName(mod_address, type_index));
//        }
//        else
//        {
//            functions.emplace_back(L"?");
//        }
//    }
//    return functions;
//}

/*std::vector<std::wstring> DbgHelp::GetFullTrace(const std::vector<StacktraceInfo>& trace)
{
    std::vector<std::wstring> full_trace(trace.size());

    DWORD64 mod_address;
    ULONG type_index;

    for (auto& i = trace.begin(); i != trace.end(); i++)
    {
        if (!GetModuleBaseAndSymIndex(i->m_address, &mod_address, &type_index) &&
            GetSymbolTag(mod_address, type_index) != SymTagFunction)
        {
            full_trace.emplace_back(L"?");
            continue;
        }

        std::wstring symbol;
        DWORD params = GetParamCountForClass(mod_address, type_index);
        DWORD class_index = GetParentClass(mod_address, type_index);
        if (params == GetParamCount(mod_address, type_index) && class_index != 0)
        {
            symbol.append(L"static ");
        }
        symbol.append(GetTypeName(mod_address, type_index));
        if (class_index != 0)
        {
            symbol.append(GetSymbolName(mod_address, class_index)).append(L"::");
        }
        symbol.append(GetSymbolName(mod_address, type_index)).append(L"(");

        full_trace.emplace_back(symbol);
    }

    return full_trace;
}*/

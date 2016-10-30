/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <array>
#include <vector>

#include "external\dbghelp\cvconst.h"
#include "external\dbghelp\dbghelp.h"

#include "DbgHelp.h"

/*
 * TODO: Remove, use a newer dbghelp.dll instead with .lib file
 */
static BOOL(__stdcall *SymInitializePointer)(HANDLE hProcess, PCSTR UserSearchPath, BOOL fInvadeProcess) = nullptr;
static BOOL(__stdcall *SymCleanupPointer)(HANDLE hProcess) = nullptr;
static DWORD(__stdcall *pSymSetOptions)(DWORD) = nullptr;
static DWORD(__stdcall *SymLoadModule64Pointer)(HANDLE hProcess, HANDLE hFile, PCSTR ImageName, PCSTR ModuleName, DWORD64 BaseOfDll, DWORD SizeOfDll) = nullptr;
static BOOL(__stdcall *SymGetModuleInfo64Pointer)(HANDLE hProcess, DWORD64 qwAddr, PIMAGEHLP_MODULE64 ModuleInfo) = nullptr;
static PVOID(__stdcall *pImageRvaToVa)(PIMAGE_NT_HEADERS, PVOID, ULONG, PIMAGE_SECTION_HEADER*) = nullptr;
static BOOL(__stdcall *SymEnumSymbolsPointer)(HANDLE hProcess, ULONG64 BaseOfDll, PCSTR Mask, PSYM_ENUMERATESYMBOLS_CALLBACK EnumSymbolsCallback, PVOID UserContext) = nullptr;
static BOOL(__stdcall *StackWalk64Pointer)(DWORD MachineType, HANDLE hProcess, HANDLE hThread, LPSTACKFRAME64 StackFrame, PVOID ContextRecord, PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine, PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine, PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine, PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress) = nullptr;
static PVOID(__stdcall *SymFunctionTableAccess64Pointer)(HANDLE hProcess, DWORD64 AddrBase) = nullptr;
static DWORD64(__stdcall *SymGetModuleBase64Pointer)(HANDLE hProcess, DWORD64 Address) = nullptr;
static BOOL(__stdcall *SymFromAddrPointer)(HANDLE hProcess, DWORD64 Address, PDWORD64 Displacement, PSYMBOL_INFO Symbol) = nullptr;
static DWORD(__stdcall *UnDecorateSymbolNamePointer)(PCSTR DecoratedName, PSTR UnDecoratedName, DWORD UndecoratedLength, DWORD Flags) = nullptr;
static BOOL(__stdcall *pSymGetLineFromAddr)(HANDLE hProcess, DWORD, PDWORD, PIMAGEHLP_LINE) = nullptr;
static BOOL(__stdcall *SymGetTypeInfoPointer)(HANDLE hProcess, DWORD64 ModBase, ULONG TypeId, IMAGEHLP_SYMBOL_TYPE_INFO GetType, PVOID pInfo) = nullptr;
static BOOL(__stdcall *SymGetTypeInfoExPointer)(HANDLE hProcess, DWORD64 ModBase, PIMAGEHLP_GET_TYPE_INFO_PARAMS Params) = nullptr;

DbgHelp::DbgHelp(HANDLE process) :
    m_dbghelp_dll(nullptr),
    m_process(process)
{
    std::array<CHAR, 0x1000> buffer;
    buffer.fill('\0');
    std::string symbol_paths;

    /*
     * TODO: Remove, use a newer dbghelp.dll instead with .lib file
     */
    GetModuleFileNameA(NULL, buffer.data(), buffer.size());
    std::string dll_path(buffer.data());
    dll_path.erase(dll_path.rfind('\\') + 1);
    symbol_paths.append(dll_path);
    dll_path.append("dbghelp.dll");
    m_dbghelp_dll = LoadLibraryA(dll_path.c_str());
    if (m_dbghelp_dll == nullptr)
    {
        throw std::exception("dbghelp.dll not found");
    }
    SymInitializePointer = reinterpret_cast<decltype(SymInitializePointer)>(GetProcAddress(m_dbghelp_dll, "SymInitialize"));
    if (SymInitializePointer == nullptr)
    {
        FreeLibrary(m_dbghelp_dll);
        throw std::exception("dbghelp.dll did not have SymInitialize");
    }
    SymCleanupPointer = reinterpret_cast<decltype(SymCleanupPointer)>(GetProcAddress(m_dbghelp_dll, "SymCleanup"));
    if (SymCleanupPointer == nullptr)
    {
        FreeLibrary(m_dbghelp_dll);
        throw std::exception("dbghelp.dll did not have SymCleanup");
    }
    SymFunctionTableAccess64Pointer = reinterpret_cast<decltype(SymFunctionTableAccess64Pointer)>(GetProcAddress(m_dbghelp_dll, "SymFunctionTableAccess64"));
    if (SymFunctionTableAccess64Pointer == nullptr)
    {
        FreeLibrary(m_dbghelp_dll);
        throw std::exception("dbghelp.dll did not have SymFunctionTableAccess64");
    }
    SymGetModuleBase64Pointer = reinterpret_cast<decltype(SymGetModuleBase64Pointer)>(GetProcAddress(m_dbghelp_dll, "SymGetModuleBase64"));
    if (SymGetModuleBase64Pointer == nullptr)
    {
        FreeLibrary(m_dbghelp_dll);
        throw std::exception("dbghelp.dll did not have SymGetModuleBase64");
    }
    SymGetModuleInfo64Pointer = reinterpret_cast<decltype(SymGetModuleInfo64Pointer)>(GetProcAddress(m_dbghelp_dll, "SymGetModuleInfo64"));
    if (SymGetModuleInfo64Pointer == nullptr)
    {
        FreeLibrary(m_dbghelp_dll);
        throw std::exception("dbghelp.dll did not have SymGetModuleInfo64");
    }
    SymLoadModule64Pointer = reinterpret_cast<decltype(SymLoadModule64Pointer)>(GetProcAddress(m_dbghelp_dll, "SymLoadModule64"));
    if (SymLoadModule64Pointer == nullptr)
    {
        FreeLibrary(m_dbghelp_dll);
        throw std::exception("dbghelp.dll did not have SymLoadModule64");
    }
    SymEnumSymbolsPointer = reinterpret_cast<decltype(SymEnumSymbolsPointer)>(GetProcAddress(m_dbghelp_dll, "SymEnumSymbols"));
    if (SymEnumSymbolsPointer == nullptr)
    {
        FreeLibrary(m_dbghelp_dll);
        throw std::exception("dbghelp.dll did not have SymEnumSymbols");
    }
    StackWalk64Pointer = reinterpret_cast<decltype(StackWalk64Pointer)>(GetProcAddress(m_dbghelp_dll, "StackWalk64"));
    if (StackWalk64Pointer == nullptr)
    {
        FreeLibrary(m_dbghelp_dll);
        throw std::exception("dbghelp.dll did not have StackWalk64");
    }
    SymFromAddrPointer = reinterpret_cast<decltype(SymFromAddrPointer)>(GetProcAddress(m_dbghelp_dll, "SymFromAddr"));
    if (SymFromAddrPointer == nullptr)
    {
        FreeLibrary(m_dbghelp_dll);
        throw std::exception("dbghelp.dll did not have SymFromAddr");
    }
    UnDecorateSymbolNamePointer = reinterpret_cast<decltype(UnDecorateSymbolNamePointer)>(GetProcAddress(m_dbghelp_dll, "UnDecorateSymbolName"));
    if (UnDecorateSymbolNamePointer == nullptr)
    {
        FreeLibrary(m_dbghelp_dll);
        throw std::exception("dbghelp.dll did not have UnDecorateSymbolName");
    }
    SymGetTypeInfoPointer = reinterpret_cast<decltype(SymGetTypeInfoPointer)>(GetProcAddress(m_dbghelp_dll, "SymGetTypeInfo"));
    if (SymGetTypeInfoPointer == nullptr)
    {
        FreeLibrary(m_dbghelp_dll);
        throw std::exception("dbghelp.dll did not have SymGetTypeInfo");
    }
    SymGetTypeInfoExPointer = reinterpret_cast<decltype(SymGetTypeInfoExPointer)>(GetProcAddress(m_dbghelp_dll, "SymGetTypeInfoEx"));
    if (SymGetTypeInfoExPointer == nullptr)
    {
        FreeLibrary(m_dbghelp_dll);
        throw std::exception("dbghelp.dll did not have SymGetTypeInfoEx");
    }
    buffer.fill('\0');
    if (GetCurrentDirectoryA(buffer.size(), buffer.data()) != 0)
    {
        symbol_paths.append(";").append(buffer.data());
    }
    if (GetEnvironmentVariableA("_NT_SYMBOL_PATH", buffer.data(), buffer.size()) != 0)
    {
        symbol_paths.append(";").append(buffer.data());
    }
    if (GetEnvironmentVariableA("_NT_ALTERNATIVE_SYMBOL_PATH", buffer.data(), buffer.size()) != 0)
    {
        symbol_paths.append(";").append(buffer.data());
    }
    if (GetEnvironmentVariableA("SYSTEMROOT", buffer.data(), buffer.size()) != 0)
    {
        symbol_paths.append(";").append(buffer.data())
                    .append(";").append(buffer.data()).append("\\system32")
                    .append(";").append(buffer.data()).append("\\SysWOW64");
    }
    /*
    * TODO: Enable when SymbolServer support is added. Needs symsrv.dll.
    * This will enable downloading of symbols for the system DLLs making stack traces more accurate.
    * -- Warepire
    */
    //symbol_paths.append(";").append("SRV*%SYSTEMDRIVE%\\websymbols*http://msdl.microsoft.com/download/symbols");
    if (SymInitializePointer(m_process, symbol_paths.c_str(), FALSE) == FALSE)
    {
        FreeLibrary(m_dbghelp_dll);
        throw std::exception("dbghelp.dll could not initialize symbol handling");
    }
}
DbgHelp::~DbgHelp()
{
    SymCleanupPointer(m_process);
    FreeLibrary(m_dbghelp_dll);
}

void DbgHelp::LoadSymbols(HANDLE module_file, LPCSTR module_name, DWORD64 module_base)
{
    DWORD64 base_address = SymLoadModule64Pointer(m_process, module_file, module_name, 0, module_base, 0);
    if (base_address != 0)
    {
        m_loaded_modules.emplace(module_base, module_name);
        SymEnumSymbolsPointer(m_process, base_address, NULL, DbgHelp::LoadSymbolsCallback, this);
    }
}

std::vector<DbgHelp::StacktraceInfo> DbgHelp::Stacktrace(HANDLE thread, INT max_depth)
{
    std::vector<DbgHelp::StacktraceInfo> trace;
    CONTEXT thread_context;
    STACKFRAME64 stack_frame;

    thread_context.ContextFlags = CONTEXT_FULL;
    if (m_loaded_modules.empty())
    {
        return trace;
    }
    if (GetThreadContext(thread, &thread_context) == FALSE)
    {
        return trace;
    }

    stack_frame.AddrPC.Offset = thread_context.Eip;
    stack_frame.AddrPC.Mode = AddrModeFlat;
    stack_frame.AddrStack.Offset = thread_context.Esp;
    stack_frame.AddrStack.Mode = AddrModeFlat;
    stack_frame.AddrFrame.Offset = thread_context.Ebp;
    stack_frame.AddrFrame.Mode = AddrModeFlat;

    for (INT i = 0; ; i++)
    {
        BOOL rv = StackWalk64Pointer(IMAGE_FILE_MACHINE_I386, m_process, thread,
                                     &stack_frame, &thread_context, nullptr,
                                     SymFunctionTableAccess64Pointer, SymGetModuleBase64Pointer,
                                     nullptr);
        if (rv == FALSE)
        {
            break;
        }
        DWORD64 address = stack_frame.AddrPC.Offset;

        /*
         * lower_bound() will return an iterator to the location in the map where 'address' should
         * be inserted, be it used with i.e. emplace_hint(). We can thus use it to look up the
         * module 'address' belongs to by getting this iterator, and then iterate backwards once.
         */
        auto module_it = m_loaded_modules.lower_bound(address);
        module_it--;
        trace.emplace_back(address, module_it->second);

        if (i >= max_depth)
        {
            break;
        }
    }
    return trace;
}

std::vector<std::wstring> DbgHelp::GetFunctionTrace(const std::vector<StacktraceInfo>& trace)
{
    std::vector<std::wstring> functions(trace.size());
    DWORD64 mod_address;
    ULONG type_index;

    for (auto& i = trace.begin(); i != trace.end(); i++)
    {
        if (GetModuleBaseAndSymIndex(i->m_address, &mod_address, &type_index) &&
            GetSymbolTag(mod_address, type_index) == SymTagFunction)
        {
            functions.emplace_back(GetSymbolName(mod_address, type_index));
        }
        else
        {
            functions.emplace_back(L"?");
        }
    }
    return functions;
}

std::vector<std::wstring> DbgHelp::GetFullTrace(const std::vector<StacktraceInfo>& trace)
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
}

BOOL CALLBACK DbgHelp::LoadSymbolsCallback(PSYMBOL_INFO symbol_info, ULONG symbol_size, PVOID user_data)
{
    return TRUE;
}

bool DbgHelp::GetModuleBaseAndSymIndex(DWORD64 address, PDWORD64 mod_base, PULONG index)
{
    if (mod_base == nullptr || index == nullptr)
    {
        return false;
    }

    BOOL rv;
    DWORD64 symbol_displacement;
    std::array<BYTE, MAX_SYM_NAME + sizeof(SYMBOL_INFO)> symbol_buffer;
    PSYMBOL_INFO symbol_info = reinterpret_cast<PSYMBOL_INFO>(symbol_buffer.data());
    symbol_info->MaxNameLen = MAX_SYM_NAME;
    symbol_info->SizeOfStruct = sizeof(SYMBOL_INFO);

    rv = SymFromAddrPointer(m_process, address, &symbol_displacement, symbol_info);
    if (rv == FALSE)
    {
        return false;
    }

    *mod_base = symbol_info->ModBase;
    *index = symbol_info->TypeIndex;

    return true;
}

DWORD DbgHelp::GetSymbolTag(DWORD64 module_base, ULONG type_index)
{
    DWORD symbol_tag = SymTagNull;
    if (SymGetTypeInfoPointer(m_process, module_base, type_index, TI_GET_SYMTAG, &symbol_tag) == FALSE)
    {
        symbol_tag = SymTagNull;
    }
    return symbol_tag;
}

DWORD DbgHelp::GetCallingConvention(DWORD64 module_base, ULONG type_index)
{
    DWORD call_conv = 0;
    if (SymGetTypeInfoPointer(m_process, module_base, type_index, TI_GET_CALLING_CONVENTION, &call_conv) == FALSE)
    {
        call_conv = 0;
    }
    return call_conv;
}

DWORD DbgHelp::GetParamCount(DWORD64 module_base, ULONG type_index)
{
    DWORD param_count = 0;
    if (SymGetTypeInfoPointer(m_process, module_base, type_index, TI_GET_COUNT, &param_count) == FALSE)
    {
        param_count = 0;
    }
    return param_count;
}

DWORD DbgHelp::GetParamCountForClass(DWORD64 module_base, ULONG type_index)
{
    DWORD param_count = 0;
    if (SymGetTypeInfoPointer(m_process, module_base, type_index, TI_GET_CHILDRENCOUNT, &param_count) == FALSE)
    {
        param_count = 0;
    }
    return param_count;
}

DWORD DbgHelp::GetParentClass(DWORD64 module_base, ULONG type_index)
{
    DWORD class_id = 0;
    if (SymGetTypeInfoPointer(m_process, module_base, type_index, TI_GET_CLASSPARENTID, &class_id) == FALSE)
    {
        class_id = 0;
    }
    return class_id;
}

DWORD DbgHelp::GetThisPtrOffset(DWORD64 module_base, ULONG type_index)
{
    DWORD offset = 0;
    if (SymGetTypeInfoPointer(m_process, module_base, type_index, TI_GET_THISADJUST, &offset) == FALSE)
    {
        offset = 0;
    }
    return offset;
}

std::wstring DbgHelp::GetTypeName(DWORD64 module_base, ULONG type_index)
{
    DWORD type_id = 0;
    if (SymGetTypeInfoPointer(m_process, module_base, type_index, TI_GET_TYPEID, &type_id))
    {
        return GetSymbolName(module_base, type_id);
    }
    return L"?";
}

std::wstring DbgHelp::GetSymbolName(DWORD64 module_base, ULONG type_index)
{
    std::wstring name(L"?");
    LPWSTR symbol_name;
    if (SymGetTypeInfoPointer(m_process, module_base, type_index, TI_GET_SYMNAME, &symbol_name) &&
        symbol_name != nullptr)
    {
        name = symbol_name;
        /*
         * Docs don't mention which function to use, 3rd party code examples seems to use
         * LocalFree(). Assume that is correct, if there are issues, try something else...
         * like GlobalFree().
         * -- Warepire
         */
        LocalFree(symbol_name);
    }
    return name;
}

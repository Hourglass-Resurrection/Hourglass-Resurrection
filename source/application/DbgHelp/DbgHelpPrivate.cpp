/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include <map>
#include <memory>
#include <string>
#include <vector>
/*
 * Default location of the DIA SDK within VS2015 Community edition.
 */
#include <../../DIA SDK/include/dia2.h>
#pragma comment(lib, "../../DIA SDK/lib/diaguids.lib")

#include "application/logging.h"
#include "application/Utils/File.h"
#include "application/Utils/COM.h"
#include "DbgHelp.h"
#include "DbgHelpLoadCallback.h"
#include "DbgHelpStackWalkHelper.h"
#include "DbgHelpStackWalkCallback.h"
#include "DbgHelpStackWalkCallbackPrivate.h"

#include "DbgHelpPrivate.h"

DbgHelpPrivate::DbgHelpPrivate(HANDLE process) :
    m_process(process),
    m_platform_set(false),
    m_should_uninitialize(true)
{
    HRESULT rv = CoInitializeEx(nullptr, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY);
    if (rv != S_OK || rv != S_FALSE)
    {
        m_should_uninitialize = false;
    }
}

DbgHelpPrivate::~DbgHelpPrivate()
{
    if (m_should_uninitialize)
    {
        CoUninitialize();
    }
}

bool DbgHelpPrivate::LoadSymbols(DWORD64 module_base, const std::wstring& exec, const std::wstring& search_path)
{
    DbgHelpLoadCallback load_callback;
    auto data_source = Utils::COM::MakeUniqueCOMPtr<IDiaDataSource>(CLSID_DiaSource);
    IDiaSession* sess = nullptr;
    IDiaSymbol* sym = nullptr;
    auto file_headers = Utils::File::ExecutableFileHeaders(exec);
    DWORD module_size_in_ram = file_headers.GetImageSizeInRAM();

    m_loaded_modules[module_base].m_module_name = exec;
    m_loaded_modules[module_base].m_module_load_address = module_base;
    m_loaded_modules[module_base].m_module_size = module_size_in_ram;

    debugprintf(L"[Hourglass][DebugSymbols] Attempting to load symbols for \"%s\"\n", exec.c_str());

    if (data_source->loadDataForExe(exec.c_str(), search_path.c_str(), &load_callback) != S_OK)
    {
        debugprintf(L"[Hourglass][DebugSymbols] No symbols found, exporting symbols.\n");
        m_loaded_modules[module_base].m_module_exports_table = file_headers.GetExportTable();
        return true;
    }

    if (data_source->openSession(&sess) != S_OK)
    {
        return false;
    }

    Utils::COM::UniqueCOMPtr<IDiaSession> session(sess);
    sess = nullptr;

    if (session->put_loadAddress(module_base) != S_OK)
    {
        return false;
    }

    if (!m_platform_set)
    {
        if (session->get_globalScope(&sym) != S_OK)
        {
            return false;
        }

        Utils::COM::UniqueCOMPtr<IDiaSymbol> symbol(sym);
        sym = nullptr;

        DWORD platform;
        HRESULT rv = symbol->get_platform(&platform);
        if (rv == S_OK)
        {
            m_platform_set = true;
            m_platform = static_cast<CV_CPU_TYPE_e>(platform);
        }
        else if (rv != S_FALSE)
        {
            debugprintf(L"symbol->get_platform failed with 0x%X", rv);
            return false;
        }
    }
    m_loaded_modules[module_base].m_module_symbol_session = std::move(session);

    return true;
}

bool DbgHelpPrivate::StackWalk(HANDLE thread, DbgHelp::StackWalkCallback& cb)
{
    auto stack_walker = Utils::COM::MakeUniqueCOMPtr<IDiaStackWalker>(CLSID_DiaStackWalker);
    IDiaEnumStackFrames* frames = nullptr;
    CONTEXT thread_context;
    thread_context.ContextFlags = CONTEXT_ALL;
    if (m_loaded_modules.empty())
    {
        return false;
    }

    if (GetThreadContext(thread, &thread_context) == FALSE)
    {
        return false;
    }
    DbgHelpStackWalkHelper helper(this, thread_context);
    /*
     * CV_CFL_PENTIUM is the flag passed to getEnumFrames2 internally by getEnumFrames.
     */
    if (stack_walker->getEnumFrames2(m_platform_set ? m_platform : CV_CFL_PENTIUM, &helper, &frames) != S_OK)
    {
        return false;
    }
    Utils::COM::UniqueCOMPtr<IDiaEnumStackFrames> stack_frames(frames);
    frames = nullptr;

    IDiaStackFrame *stack_frame = nullptr;
    for (ULONG next = 0; (stack_frames->Next(1, &stack_frame, &next) == S_OK) && (next == 1);)
    {
        ULONGLONG pc;
        stack_frame->get_registerValue(CV_REG_EIP, &pc);
        auto mod_info = GetModuleData(pc);
        auto rv = cb(DbgHelpStackWalkCallback(new DbgHelpStackWalkCallbackPrivate(stack_frame, mod_info)));
        stack_frame->Release();
        if (rv == DbgHelpStackWalkCallback::Action::STOP)
        {
            break;
        }
    }

    return true;
}

HANDLE DbgHelpPrivate::GetProcess() const
{
    return m_process;
}

const DbgHelpPrivate::ModuleData* DbgHelpPrivate::GetModuleData(ULONGLONG virtual_address) const
{
    /*
     * lower_bound() will return an iterator to the location in the map where 'address' should
     * be inserted, be it used with i.e. emplace_hint(). We can thus use it to look up the
     * DataSource tied to 'virtual_address' by checking the position before.
     */
    auto it = m_loaded_modules.lower_bound(virtual_address);
    if (it == m_loaded_modules.begin() || (--it) == m_loaded_modules.begin())
    {
        return nullptr;
    }
    LONGLONG rva = virtual_address - it->first;
    if (rva >= 0 && it->second.m_module_size >= rva)
    {
        return &(it->second);
    }
    return nullptr;
}

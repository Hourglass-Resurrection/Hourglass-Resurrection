/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <atlbase.h>
#include <atlcom.h>

#include <DIA SDK/include/dia2.h>

#include "application/logging.h"
#include "application/Utils/COM.h"
#include "application/Utils/File.h"
#include "DbgHelpLoadCallback.h"
#include "DbgHelpStackWalkHelper.h"
#include "DbgHelpStackWalkCallback.h"

#include "DbgHelpPrivate.h"

DbgHelpPrivate::DbgHelpPrivate(HANDLE process) :
    m_process(process),
    m_platform_set(false)
{
}

DbgHelpPrivate::~DbgHelpPrivate() = default;

bool DbgHelpPrivate::LoadSymbols(DWORD64 module_base, const std::wstring& exec, const std::wstring& search_path)
{
    DbgHelpLoadCallback load_callback;
    auto data_source = Utils::COM::CreateCOMPtr<IDiaDataSource>(CLSID_DiaSource);
    CComPtr<IDiaSession> session;
    CComPtr<IDiaSymbol> symbol;
    auto file_headers = Utils::File::ExecutableFileHeaders(exec);
    DWORD module_size_in_ram = file_headers.GetImageSizeInRAM();

    m_loaded_modules[module_base].m_module_name = exec;
    m_loaded_modules[module_base].m_module_size = module_size_in_ram;

    debugprintf(L"[Hourglass][DebugSymbols] Attempting to load symbols for \"%s\"\n", exec.c_str());

    if (data_source->loadDataForExe(exec.c_str(), search_path.c_str(), &load_callback) != S_OK)
    {
        debugprintf(L"[Hourglass][DebugSymbols] No symbols found, using export table as symbols.\n");
        m_loaded_modules[module_base].m_module_exports_table = file_headers.GetExportTable(module_base);
        return true;
    }

    if (data_source->openSession(&session) != S_OK)
    {
        return false;
    }

    if (session->put_loadAddress(module_base) != S_OK)
    {
        return false;
    }

    if (!m_platform_set)
    {
        if (session->get_globalScope(&symbol) != S_OK)
        {
            return false;
        }

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
    auto stack_walker = Utils::COM::CreateCOMPtr<IDiaStackWalker>(CLSID_DiaStackWalker);
    CComPtr<IDiaEnumStackFrames> stack_frames;
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
    if (stack_walker->getEnumFrames2(m_platform_set ? m_platform : CV_CFL_PENTIUM, &helper, &stack_frames) != S_OK)
    {
        return false;
    }

    IDiaStackFrame *stack_frame = nullptr;
    for (ULONG next = 0; (stack_frames->Next(1, &stack_frame, &next) == S_OK) && (next == 1);)
    {
        ULONGLONG pc;
        stack_frame->get_registerValue(CV_REG_EIP, &pc);
        if (pc == 0)
        {
            break;
        }
        auto mod_info = GetModuleData(pc);
        DbgHelpStackWalkCallback cb_data(m_process, stack_frame, mod_info);
        auto rv = cb(cb_data);
        stack_frame->Release();
        if (rv == IDbgHelpStackWalkCallback::Action::STOP)
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
    if (it == m_loaded_modules.begin())
    {
        return nullptr;
    }
    it--;
    LONGLONG rva = virtual_address - it->first;
    if (rva >= 0 && it->second.m_module_size >= rva)
    {
        return &(it->second);
    }
    return nullptr;
}

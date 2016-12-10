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

#include "../logging.h"
#include "../Utils/COM.h"
#include "DbgHelp.h"
#include "DbgHelpLoadCallback.h"
#include "DbgHelpStackWalkHelper.h"

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

    debugprintf(L"[Hourglass][DebugSymbols] Attempting to load symbols for \"%s\"n", exec.c_str());

    if (data_source->loadDataForExe(exec.c_str(), search_path.c_str(), &load_callback) != S_OK)
    {
        return false;
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
    m_loaded_modules.emplace(module_base, exec);
    m_sources.emplace(module_base, std::move(data_source));
    m_sessions.emplace(data_source.get(), std::move(session));

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

    ULONG humbug;
    IDiaStackFrame *bah;
    stack_frames->Next(1, &bah, &humbug);

    return true;
}

HANDLE DbgHelpPrivate::GetProcess() const
{
    return m_process;
}

IDiaDataSource* DbgHelpPrivate::GetDiaDataSource(DWORD64 virtual_address) const
{
    /*
    * lower_bound() will return an iterator to the location in the map where 'address' should
    * be inserted, be it used with i.e. emplace_hint(). We can thus use it to look up the
    * DataSource tied to 'virtual_address' by checking the position before.
    */
    auto it = m_sources.lower_bound(virtual_address);
    if (it == m_sources.begin() || (--it) == m_sources.begin())
    {
        return nullptr;
    }
    return it->second.get();
}

IDiaSession* DbgHelpPrivate::GetDiaSession(IDiaDataSource* source) const
{
    auto it = m_sessions.find(source);
    if (it == m_sessions.end())
    {
        return nullptr;
    }
    return it->second.get();
}

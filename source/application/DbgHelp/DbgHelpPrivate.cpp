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

#include "../Utils/COM.h"
#include "DbgHelp.h"
#include "DbgHelpStackWalkHelper.h"

#include "DbgHelpPrivate.h"

DbgHelpPrivate::DbgHelpPrivate(HANDLE process) :
    m_process(process),
    m_platform_set(false)
{
}

bool DbgHelpPrivate::LoadSymbols(DWORD64 module_base, const std::wstring& exec, const std::wstring& search_path)
{
    auto data_source = Utils::COM::MakeUniqueCOMPtr<IDiaDataSource>(CLSID_DiaSource);
    IDiaSession* sess = nullptr;
    IDiaSymbol* sym = nullptr;

    if (data_source->loadDataForExe(exec.c_str(), search_path.c_str(), nullptr) != S_OK)
    {
        return false;
    }

    if (data_source->openSession(&sess) != S_OK)
    {
        return false;
    }

    Utils::COM::UniqueCOMPtr<IDiaSession> session(sess);
    sess = nullptr;

    if (session->get_globalScope(&sym) != S_OK)
    {
        return false;
    }

    Utils::COM::UniqueCOMPtr<IDiaSymbol> symbol(sym);
    sym = nullptr;

    if (!m_platform_set)
    {
        DWORD platform;
        if (symbol->get_platform(&platform) != S_OK)
        {
            return false;
        }
        m_platform_set = true;
        m_platform = static_cast<CV_CPU_TYPE_e>(platform);
    }
    m_sources.emplace(module_base, std::move(data_source));
    m_sessions.emplace(data_source.get(), std::move(session));
    m_symbols.emplace(session.get(), std::move(symbol));

    return true;
}

bool DbgHelpPrivate::Stacktrace(HANDLE thread, INT max_depth, std::vector<DbgHelp::StackFrameInfo>* trace)
{
    auto stack_walker = Utils::COM::MakeUniqueCOMPtr<IDiaStackWalker>(CLSID_DiaStackWalker);
    IDiaEnumStackFrames* frames = nullptr;
    CONTEXT thread_context;
    thread_context.ContextFlags = CONTEXT_FULL | CONTEXT_DEBUG_REGISTERS | CONTEXT_EXTENDED_REGISTERS;
    if (!m_platform_set)
    {
        return false;
    }
    if (GetThreadContext(thread, &thread_context) == FALSE)
    {
        return false;
    }

    if (stack_walker->getEnumFrames2(m_platform, nullptr, &frames) != S_OK)
    {
        return false;
    }
    Utils::COM::UniqueCOMPtr<IDiaEnumStackFrames> stack_frames(frames);
    frames = nullptr;


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

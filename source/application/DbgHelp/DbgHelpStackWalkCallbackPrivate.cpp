/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

#include "application/logging.h"
#include "DbgHelpPrivate.h"
#include "DbgHelpStackWalkCallbackPrivate.h"

DbgHelpStackWalkCallbackPrivate::DbgHelpStackWalkCallbackPrivate(IDiaStackFrame* frame, const DbgHelpPrivate::ModuleData* mod_info) :
    m_frame(frame),
    m_mod_info(mod_info)
{
}

std::wstring DbgHelpStackWalkCallbackPrivate::GetModuleName()
{
    if (m_mod_info == nullptr)
    {
        return L"?";
    }
    return m_mod_info->m_module_name;
}

std::wstring DbgHelpStackWalkCallbackPrivate::GetFunctionName()
{
    if (m_mod_info == nullptr || m_mod_info->m_module_symbol_session == nullptr)
    {
        return L"?";
    }
    auto symbol = GetFunctionSymbol();
    std::wstring function_name = L"?";
    BSTR name = nullptr;
    if (symbol != nullptr && symbol->get_name(&name) == S_OK)
    {
        function_name = name;
        SysFreeString(name);
    }
    return function_name;
}

Utils::COM::UniqueCOMPtr<IDiaSymbol> DbgHelpStackWalkCallbackPrivate::GetFunctionSymbol()
{
    ULONGLONG pc;
    m_frame->get_registerValue(CV_REG_EIP, &pc);
    IDiaSymbol* symbol = nullptr;
    HRESULT rv = m_mod_info->m_module_symbol_session->findSymbolByVA(pc, SymTagFunction, &symbol);
    return Utils::COM::UniqueCOMPtr<IDiaSymbol>(symbol);
}

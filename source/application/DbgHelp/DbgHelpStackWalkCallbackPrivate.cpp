/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <sstream>
#include <string>

#include <atlbase.h>
#include <atlcom.h>

#include "application/logging.h"
#include "DbgHelpPrivate.h"
#include "DbgHelpStackWalkCallbackPrivate.h"

DbgHelpStackWalkCallbackPrivate::DbgHelpStackWalkCallbackPrivate(HANDLE process, IDiaStackFrame* frame, const DbgHelpPrivate::ModuleData* mod_info) :
    m_process(process),
    m_frame(frame),
    m_mod_info(mod_info),
    m_unsure(0),
    m_params_enumerated(false)
{
}

// TODO: Split module name and PC
std::wstring DbgHelpStackWalkCallbackPrivate::GetModuleName()
{
    std::wstringstream name;
    if (m_mod_info == nullptr)
    {
        name << L"?";
    }
    else
    {
        name << m_mod_info->m_module_name;
    }
    name << L"!0x" << std::hex << GetProgramCounter();
    return name.str();
}

std::wstring DbgHelpStackWalkCallbackPrivate::GetFunctionName()
{
    if (m_mod_info == nullptr)
    {
        return L"?";
    }
    std::wstring function_name = L"?";
    if (m_mod_info->m_module_symbol_session == nullptr)
    {
        auto it = m_mod_info->m_module_exports_table.lower_bound(GetProgramCounter());
        if (it != m_mod_info->m_module_exports_table.begin() && (--it) != m_mod_info->m_module_exports_table.begin())
        {
            function_name = it->second;
        }
        return function_name;
    }
    auto symbol = GetFunctionSymbol();
    
    BSTR name = nullptr;
    if (symbol != nullptr && symbol->get_name(&name) == S_OK)
    {
        function_name = name;
        SysFreeString(name);
    }
    return function_name;
}

// TODO: RFC
std::wstring DbgHelpStackWalkCallbackPrivate::GetFunctionParameters()
{
    if (m_mod_info == nullptr)
    {
        return L"?";
    }

    ULONGLONG stackframe_base = 0;
    bool can_get_values = (m_frame->get_base(&stackframe_base) == S_OK);

    DWORD param_count = GetParameterCount();
    std::wstring params = L"(";
    //if (m_mod_info->m_module_symbol_session == nullptr)
    {
        /*
         * We don't know the parameter names, nor types, nor the actual count, but we have a guess.
         * We'll also assume that there's no more than 26 parameters to a function.
         * We count backwards here, as parameters are pushed right-to-left and stacks grow downwards.
         */
        for (DWORD i = 0; /*i < param_count && */i < 26; i++)
        {
            params += ('a' + i);
            if (can_get_values)
            {
                ULONGLONG address = stackframe_base + (i * 4);
                DWORD value;
                SIZE_T read_bytes;
                if (ReadProcessMemory(m_process, reinterpret_cast<LPVOID>(address), &value, sizeof(value), &read_bytes) == TRUE)
                {
                    debugprintf(L"%#llX: %#X\n", address, value);
                    //params += L"=0x";
                    //std::wstringstream hex_value;
                    //hex_value << std::hex << value;
                    //params += hex_value.str();
                    //if (i < param_count - 1)
                    //{
                    //    params += L", ";
                    //}
                }
            }
        }
        params += L")?";
    }
    //else
    //{
    //    for (DWORD i = param_count; i > 0; i--)
    //    {

    //    }
    //    params += L")";
    //}
    return params;
}

DWORD DbgHelpStackWalkCallbackPrivate::GetUnsureStatus()
{
    return m_unsure;
}

DWORD DbgHelpStackWalkCallbackPrivate::GetParameterCount()
{
    if (!m_params_enumerated)
    {
        EnumerateParameters();
    }

    return m_param_info.size();
}

CComPtr<IDiaSymbol> DbgHelpStackWalkCallbackPrivate::GetFunctionSymbol()
{
    CComPtr<IDiaSymbol> symbol;
    HRESULT rv = m_mod_info->m_module_symbol_session->findSymbolByVA(GetProgramCounter(), SymTagFunction, &symbol);
    return symbol;
}

ULONGLONG DbgHelpStackWalkCallbackPrivate::GetProgramCounter()
{
    ULONGLONG pc;
    m_frame->get_registerValue(CV_REG_EIP, &pc);
    return pc;
}

// TODO: RFC
void DbgHelpStackWalkCallbackPrivate::EnumerateParameters()
{
    if (m_mod_info->m_module_symbol_session == nullptr)
    {
        /*
         * TODO: Look at callee asm to guess number of parameters
         *       this always evaluates to 0 when no debug symbols are known...
         */
    }
    else
    {
        auto symbol = GetFunctionSymbol();
        CComPtr<IDiaEnumSymbols> enum_symbols;
        HRESULT rv = symbol->findChildren(SymTagNull, nullptr, nsNone, &enum_symbols);
        if (enum_symbols == nullptr)
        {
            return;
        }
        IDiaSymbol* sym_info;
        ULONG num_fetched;

        /*
         * This will be incremented for each found symbol, with it's size.
         */
        DWORD offset = 0;
        while (enum_symbols->Next(1, &sym_info, &num_fetched) == S_OK)
        {
            if (num_fetched != 1)
            {
                sym_info->Release();
                continue;
            }
            CComPtr<IDiaSymbol> type_info;
            if (sym_info->get_type(&type_info) != S_OK)
            {
                /*
                 * This will filter out most non-argument children in the function
                 */
                sym_info->Release();
                continue;
            }
            DWORD symtag;
            if (sym_info->get_symTag(&symtag) != S_OK || symtag == SymTagCallSite)
            {
                sym_info->Release();
                continue;
            }
            //DWORD tag;
            //if (type_info->get_symTag(&tag) != S_OK)
            //{
            //    sym_info->Release();
            //    continue;
            //}
            ULONGLONG length = 0;
            if (type_info->get_length(&length) != S_OK)
            {
                sym_info->Release();
                continue;
            }

            BSTR name;
            if (sym_info->get_name(&name) != S_OK)
            {
                name = nullptr;
            }
            /*
             * The cast should be safe, if a parameter on the stack needs more than 4 GB of memory
             * something else has probably gone really awry.
             * -- Warepire
             */
            m_param_info.emplace_back(ParamInfo{ name != nullptr ? name : L"",
                                                 sym_info, type_info,
                                                 static_cast<DWORD>(length), offset });
            offset += static_cast<DWORD>(length);
            // Temp debug stuff
            //debugprintf(L"Found argument %s with type tag %u\n", name, tag);

            if (name != nullptr)
            {
                SysFreeString(name);
            }
            sym_info->Release();
        }
    }
    /*ULONGLONG stackframe_base = 0;
    m_frame->get_base(&stackframe_base);
    for (DWORD i = 0; i < 30; i++)
    {
        ULONGLONG address = stackframe_base + (i * 4);
        DWORD value;
        SIZE_T read_bytes;
        if (ReadProcessMemory(m_process, reinterpret_cast<LPVOID>(address), &value, sizeof(value), &read_bytes) == TRUE)
        {
            debugprintf(L"%#llX: %#X\n", address, value);
        }
    }*/
    m_params_enumerated = true;
}

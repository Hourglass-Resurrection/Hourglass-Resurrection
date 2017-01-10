/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

#include <atlbase.h>
#include <atlcom.h>
#include <../../DIA SDK/include/dia2.h>

#include "DbgHelpPrivate.h"

class DbgHelpStackWalkCallbackPrivate
{
public:
    DbgHelpStackWalkCallbackPrivate(HANDLE process, IDiaStackFrame* frame, const DbgHelpPrivate::ModuleData* mod_info);

    std::wstring GetModuleName();
    std::wstring GetFunctionName();
    DWORD GetParameterCount();
    std::wstring GetFunctionParameters(); // TODO: Destroy

    DWORD GetUnsureStatus();
private:
    /*
     * These functions are just helper methods and will do very little data validation
     * Always validate data before calling them.
     */
    CComPtr<IDiaSymbol> GetFunctionSymbol();
    ULONGLONG GetProgramCounter();
    void EnumerateParameters();

    HANDLE m_process;
    IDiaStackFrame* m_frame;
    const DbgHelpPrivate::ModuleData* m_mod_info;

    /*
     * TODO: Cache more?
     * -- Warepire
     */
    struct ParamInfo
    {
        std::wstring m_name;
        CComPtr<IDiaSymbol> m_arg_info;
        CComPtr<IDiaSymbol> m_type_info;
        DWORD m_size;
        DWORD m_offset;
    };

    DWORD m_unsure;
    CV_call_e m_call_conv; // TODO: own enum
    std::vector<ParamInfo> m_param_info;

    bool m_params_enumerated;
};

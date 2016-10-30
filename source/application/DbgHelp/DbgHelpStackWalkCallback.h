/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <memory>
#include <string>

#include <atlbase.h>
#include <atlcom.h>
#include <DIA SDK/include/dia2.h>

#include "DbgHelpPrivate.h"
#include "DbgHelpTypes.h"
#include "IDbgHelpStackWalkCallback.h"

class DbgHelpStackWalkCallback : public IDbgHelpStackWalkCallback
{
public:
    DbgHelpStackWalkCallback(HANDLE process, IDiaStackFrame* frame, const DbgHelpPrivate::ModuleData* mod_info);

    const std::wstring GetModuleName();
    const std::wstring GetFunctionName();
    const DWORD GetParameterCount();
    DbgHelpArgType GetParameterType(DWORD num);
    std::wstring GetParameterTypeName(DWORD num);
    std::wstring GetParameterName(DWORD num);
    std::shared_ptr<void> GetParameterValue(DWORD num);
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
        DbgHelpArgType m_type;
        std::wstring m_typename;
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

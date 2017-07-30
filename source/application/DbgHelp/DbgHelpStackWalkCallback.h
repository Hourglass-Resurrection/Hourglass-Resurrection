/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

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

    ULONGLONG GetProgramCounter() override;
    std::wstring GetModuleName() override;
    std::wstring GetFunctionName() override;
    std::vector<Parameter> GetParameters() override;
    DWORD GetUnsureStatus() override;

private:
    /*
     * TODO: Cache more?
     * -- Warepire
     */
    struct ParamInfo
    {
        DbgHelpType m_type;
        std::optional<std::wstring> m_name;
        CComPtr<IDiaSymbol> m_arg_info;
        CComPtr<IDiaSymbol> m_type_info;
        size_t m_offset;
    };

    /*
     * These functions are just helper methods and will do very little data validation.
     * Always validate data before calling them.
     */
    CComPtr<IDiaSymbol> GetFunctionSymbol();
    void EnumerateParameters();
    std::optional<ParameterValue> GetParameterValue(const ParamInfo& param_info);

    HANDLE m_process;
    IDiaStackFrame* m_frame;
    const DbgHelpPrivate::ModuleData* m_mod_info;

    DWORD m_unsure;
    std::vector<ParamInfo> m_param_info;

    bool m_params_enumerated;
};

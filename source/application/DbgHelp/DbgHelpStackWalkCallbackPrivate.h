/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

#include <../../DIA SDK/include/dia2.h>

#include "application/Utils/COM.h"
#include "DbgHelpPrivate.h"

class DbgHelpStackWalkCallbackPrivate
{
public:
    DbgHelpStackWalkCallbackPrivate(IDiaStackFrame* frame, const DbgHelpPrivate::ModuleData* mod_info);

    std::wstring GetModuleName();
    std::wstring GetFunctionName();
private:
    Utils::COM::UniqueCOMPtr<IDiaSymbol> GetFunctionSymbol();

    IDiaStackFrame* m_frame;
    const DbgHelpPrivate::ModuleData* m_mod_info;
};

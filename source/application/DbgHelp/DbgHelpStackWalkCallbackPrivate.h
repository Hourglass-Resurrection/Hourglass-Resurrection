/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <../../DIA SDK/include/dia2.h>

class DbgHelpStackWalkCallbackPrivate
{
public:
    DbgHelpStackWalkCallbackPrivate(IDiaStackFrame* frame, IDiaSymbol* symbol);

private:
    IDiaStackFrame* m_frame;
    IDiaSymbol* m_symbol;
};

/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "DbgHelpStackWalkCallbackPrivate.h"

DbgHelpStackWalkCallbackPrivate::DbgHelpStackWalkCallbackPrivate(IDiaStackFrame* frame, IDiaSymbol* symbol) :
    m_frame(frame),
    m_symbol(symbol)
{
}

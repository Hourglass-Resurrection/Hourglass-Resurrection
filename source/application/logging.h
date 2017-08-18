/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Copyright (c) 2011 nitsuja and contributors
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <sstream>

#include "DbgHelp/DbgHelp.h"

/*
 * TODO: Categorization of messages
 * -- Warepire
 */
class DebugLog
{
public:
    ~DebugLog();

    template<class T>
    DebugLog& operator<<(const T& value)
    {
        m_buffer << value;
        return *this;
    }
private:
    std::wostringstream m_buffer;
};

class VerboseDebugLog
{
public:
    template<class T>
    VerboseDebugLog& operator<<(const T& value)
    {
#ifdef _DEBUG
        m_buffer << value;
#endif
        return *this;
    }
private:
    DebugLog m_buffer;
};

// Must be called once and ONCE only, recommended to make the call from the initialization code of the whole program!
void InitDebugCriticalSection();

void PrintLastError(LPCWSTR lpszFunction, DWORD dw);

IDbgHelpStackWalkCallback::Action PrintStackTrace(IDbgHelpStackWalkCallback& data);

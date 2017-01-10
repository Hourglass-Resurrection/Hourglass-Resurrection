/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

class DbgHelpStackWalkCallbackPrivate;

class DbgHelpStackWalkCallback
{
public:
    enum class Action
    {
        CONTINUE,
        STOP,
    };

    DbgHelpStackWalkCallback(DbgHelpStackWalkCallbackPrivate* priv);
    ~DbgHelpStackWalkCallback();

    // TODO: Methods
    std::wstring GetModuleName() const;
    std::wstring GetFunctionName() const;
    DWORD GetParameterCount() const;
private:
    DbgHelpStackWalkCallbackPrivate* m_priv;
};

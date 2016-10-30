/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

class IDbgHelpStackWalkCallback
{
public:
    enum class Action
    {
        CONTINUE,
        STOP,
    };

    virtual ~IDbgHelpStackWalkCallback() = default;

    // TODO: Methods
    virtual const std::wstring GetModuleName() = 0;
    virtual const std::wstring GetFunctionName() = 0;
    virtual const DWORD GetParameterCount() = 0;
};

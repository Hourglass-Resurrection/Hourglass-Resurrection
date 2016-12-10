/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <memory>

class DbgHelpStackWalkCallbackPrivate;

class DbgHelpStackWalkCallback
{
public:
    enum class Action
    {
        CONTINUE,
        STOP,
    };

    DbgHelpStackWalkCallback(std::unique_ptr<DbgHelpStackWalkCallbackPrivate> priv);

    // TODO: Methods
private:
    std::unique_ptr<DbgHelpStackWalkCallbackPrivate> m_priv;
};

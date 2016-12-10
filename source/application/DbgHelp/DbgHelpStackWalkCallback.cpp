/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <memory>

#include "DbgHelpStackWalkCallbackPrivate.h"

#include "DbgHelpStackWalkCallback.h"

DbgHelpStackWalkCallback::DbgHelpStackWalkCallback(std::unique_ptr<DbgHelpStackWalkCallbackPrivate> priv)
{
    m_priv.swap(priv);
}

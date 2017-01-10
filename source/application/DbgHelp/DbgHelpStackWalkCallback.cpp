/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "DbgHelpStackWalkCallbackPrivate.h"

#include "DbgHelpStackWalkCallback.h"

DbgHelpStackWalkCallback::DbgHelpStackWalkCallback(DbgHelpStackWalkCallbackPrivate* priv) :
    m_priv(priv)
{
}

DbgHelpStackWalkCallback::~DbgHelpStackWalkCallback()
{
    delete m_priv;
}

 std::wstring DbgHelpStackWalkCallback::GetModuleName() const
 {
     return m_priv->GetModuleName();
 }

 std::wstring DbgHelpStackWalkCallback::GetFunctionName() const
 {
     return m_priv->GetFunctionName();
 }

 DWORD DbgHelpStackWalkCallback::GetParameterCount() const
 {
     return m_priv->GetParameterCount();
 }

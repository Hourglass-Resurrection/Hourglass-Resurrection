/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

#include "../Core/ObjBase.h"
#include "../Core/WindowClasses.h"

#include "EditControl.h"

EditControl::EditControl(const std::wstring& title,
                         short x,
                         short y,
                         short w,
                         short h,
                         DlgBase* dlg)
    : ObjBase(title,
              EDIT_WND_CLASS,
              0,
              WS_GROUP | WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL,
              x,
              y,
              w,
              h,
              dlg)
{
}

EditControl& EditControl::SetEnabled(bool enabled)
{
    SetUnsetStyleBits(0, ES_READONLY, enabled ? SetBits::Unset : SetBits::Set);
    SetUnsetStyleBits(0, WS_TABSTOP, enabled ? SetBits::Set : SetBits::Unset);
    return *this;
}

EditControl& EditControl::SetMultiLine()
{
    SetUnsetStyleBits(0, WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN, SetBits::Set);
    return *this;
}

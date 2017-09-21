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

#include "PushButton.h"

PushButton::PushButton(const std::wstring& title, short x, short y, short w, short h, DlgBase* dlg)
    : ObjBase(title, BUTTON_WND_CLASS, 0, WS_GROUP | WS_TABSTOP, x, y, w, h, dlg)
{
}

PushButton& PushButton::SetDefaultChoice()
{
    SetUnsetStyleBits(0, BS_DEFPUSHBUTTON, SetBits::Set);
    return *this;
}

PushButton& PushButton::RegisterOnClickCallback(std::function<bool(WORD)> cb)
{
    RegisterWmCommandHandler(cb);
    return *this;
}

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

#include "CheckboxButton.h"

CheckboxButton::CheckboxButton(const std::wstring & title, short x, short y, short w, short h, DlgBase* dlg) :
    ObjBase(title, BUTTON_WND_CLASS, 0, WS_GROUP | WS_TABSTOP | BS_AUTOCHECKBOX, x, y, w, h, dlg)
{
}

CheckboxButton& CheckboxButton::SetRightHand()
{
    SetUnsetStyleBits(0, BS_RIGHTBUTTON, SetBits::Set);
    return *this;
}

CheckboxButton& CheckboxButton::RegisterOnClickCallback(std::function<bool(WORD)> cb)
{
    RegisterWmCommandHandler(cb);
    return *this;
}

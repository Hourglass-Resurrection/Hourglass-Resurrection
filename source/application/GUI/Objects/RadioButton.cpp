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

#include "RadioButton.h"

RadioButton::RadioButton(const std::wstring & title, short x, short y, short w, short h, DlgBase* dlg) :
    ObjBase(title, BUTTON_WND_CLASS, 0, WS_TABSTOP | BS_AUTORADIOBUTTON, x, y, w, h, dlg)
{
}

RadioButton& RadioButton::SetAsNewGroup()
{
    SetUnsetStyleBits(0, WS_GROUP, SetBits::Unset);
    return *this;
}

RadioButton& RadioButton::SetRightHand()
{
    SetUnsetStyleBits(0, BS_RIGHTBUTTON, SetBits::Set);
    return *this;
}

RadioButton& RadioButton::SetMarked(bool marked)
{
    if (marked)
    {
        SendDlgMessage(BM_SETCHECK, BST_CHECKED, 0);
    }
    return *this;
}

RadioButton& RadioButton::RegisterOnClickCallback(std::function<bool(WORD)> cb)
{
    RegisterWmCommandHandler(cb);
    return *this;
}

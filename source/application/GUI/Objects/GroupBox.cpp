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

#include "GroupBox.h"

GroupBox::GroupBox(const std::wstring & title, short x, short y, short w, short h, DlgBase* dlg) :
    ObjBase(title, BUTTON_WND_CLASS, WS_EX_TRANSPARENT, BS_GROUPBOX, x, y, w, h, dlg)
{
}

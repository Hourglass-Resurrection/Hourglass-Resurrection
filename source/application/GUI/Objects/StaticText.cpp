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

#include "StaticText.h"

StaticText::StaticText(const std::wstring& title, short x, short y, short w, short h, DlgBase* dlg)
    : ObjBase(title, STATIC_WND_CLASS, 0, SS_LEFT, x, y, w, h, dlg)
{
}

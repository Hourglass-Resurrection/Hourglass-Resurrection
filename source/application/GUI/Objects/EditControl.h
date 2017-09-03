/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

#include "../Core/ObjBase.h"
#include "../Core/WindowClasses.h"

class DlgBase;

class EditControl : public ObjBase<EditControl, ARRAYSIZE(EDIT_WND_CLASS)>
{
public:
    EditControl(const std::wstring& title, short x, short y, short w, short h, DlgBase* dlg);

    EditControl& SetEnabled(bool enabled);
    EditControl& SetMultiLine();
};

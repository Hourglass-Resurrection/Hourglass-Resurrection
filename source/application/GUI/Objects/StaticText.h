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

class StaticText : public ObjBase<StaticText, ARRAYSIZE(STATIC_WND_CLASS)>
{
public:
    StaticText(const std::wstring& title, short x, short y, short w, short h, DlgBase* dlg);
};

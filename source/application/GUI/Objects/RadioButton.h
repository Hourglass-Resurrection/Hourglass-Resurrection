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

class RadioButton : public ObjBase<RadioButton, ARRAYSIZE(BUTTON_WND_CLASS)>
{
public:
    RadioButton(const std::wstring& title, short x, short y, short w, short h, DlgBase* dlg);

    RadioButton& SetAsNewGroup();
    RadioButton& SetRightHand();

    RadioButton& SetMarked(bool marked);

    RadioButton& RegisterOnClickCallback(std::function<bool(WORD)> cb);
};

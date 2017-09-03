/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

#include "../Core/MenuItemBase.h"

class DlgBase;

class SubMenu : public MenuItemBase
{
public:
    SubMenu(const std::wstring& title, MenuBase* parent, DlgBase* dlg);

    SubMenu& SetEnabled(bool enabled);

    SubMenu& ChangeTitle(const std::wstring& title);
};

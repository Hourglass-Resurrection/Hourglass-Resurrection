/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <functional>
#include <string>

#include "../Core/MenuItemBase.h"

class DlgBase;

class CheckableMenuItem : public MenuItemBase
{
public:
    CheckableMenuItem(const std::wstring& title,
                      const std::wstring& shortcut,
                      MenuBase* parent,
                      DlgBase* dlg);

    CheckableMenuItem& SetEnabled(bool enabled);
    CheckableMenuItem& SetChecked(bool checked);

    CheckableMenuItem& ChangeTitle(const std::wstring& title);
    CheckableMenuItem& ChangeShortcut(const std::wstring& shortcut);

    CheckableMenuItem& RegisterOnClickHandler(std::function<bool(WORD)>& cb);
};

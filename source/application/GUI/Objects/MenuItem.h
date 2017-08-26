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

class MenuItem : public MenuItemBase
{
public:
    MenuItem(const std::wstring& title, const std::wstring& shortcut, MenuItemBase* parent, DlgBase* dlg);

    MenuItem& SetEnabled(bool enabled);

    MenuItem& ChangeTitle(const std::wstring& title);
    MenuItem& ChangeShortcut(const std::wstring& shortcut);

    MenuItem& RegisterOnClickHandler(std::function<bool(WORD)> cb);
};

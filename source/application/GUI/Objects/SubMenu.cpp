/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

#include "../Core/MenuItemBase.h"

#include "SubMenu.h"

class DlgBase;

SubMenu::SubMenu(const std::wstring& title, MenuBase* parent, DlgBase* dlg)
    : MenuItemBase(title, L"", true, parent, dlg)
{
}

SubMenu& SubMenu::SetEnabled(bool enabled)
{
    SetUnsetStyleBits(MFS_DISABLED, enabled ? SetBits::Unset : SetBits::Set);
    return *this;
}

SubMenu& SubMenu::ChangeTitle(const std::wstring& title)
{
    MenuItemBase::ChangeTitle(title);
    return *this;
}

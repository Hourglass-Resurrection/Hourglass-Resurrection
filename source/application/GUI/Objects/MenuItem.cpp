/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cassert>
#include <functional>
#include <string>

#include "../Core/MenuItemBase.h"

#include "MenuItem.h"

class DlgBase;

MenuItem::MenuItem(const std::wstring& title,
                   const std::wstring& shortcut,
                   MenuItemBase* parent,
                   DlgBase* dlg)
    : MenuItemBase(title, shortcut, false, parent, dlg)
{
    assert(!title.empty());
}

MenuItem& MenuItem::SetEnabled(bool enable)
{
    SetUnsetStyleBits(MFS_DISABLED, enable ? SetBits::Unset : SetBits::Set);
    return *this;
}

MenuItem& MenuItem::ChangeTitle(const std::wstring& title)
{
    MenuItemBase::ChangeTitle(title);
    return *this;
}

MenuItem& MenuItem::ChangeShortcut(const std::wstring& shortcut)
{
    MenuItemBase::ChangeShortcut(shortcut);
    return *this;
}

MenuItem& MenuItem::RegisterOnClickHandler(std::function<bool(WORD)> cb)
{
    RegisterWmCommandHandler(cb);
    return *this;
}

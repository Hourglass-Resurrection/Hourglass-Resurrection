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
#include <vector>

#include "MenuBase.h"

class DlgBase;

class MenuItemBase : public MenuBase
{
protected:
    enum class SetBits
    {
        Set,
        Unset,
    };

    MenuItemBase(const std::wstring& title, const std::wstring& shortcut, bool submenu, MenuBase* parent, DlgBase* dlg);

    void SetUnsetStyleBits(DWORD style, SetBits set);

    void ChangeTitle(const std::wstring& title);
    void ChangeShortcut(const std::wstring& shortcut);

    void RegisterWmCommandHandler(std::function<bool(WORD)>& cb);

private:
    void ChangeText(const std::wstring& new_text);

    std::wstring m_title;
    std::wstring m_shortcut;

    DlgBase* m_dlg;
};

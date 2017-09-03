/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cassert>

#include "DlgBase.h"
#include "MenuItemBase.h"

namespace
{
#pragma pack(push, 1)
    struct MENUEX_TEMPLATE_ITEM
    {
        DWORD dwType;
        DWORD dwState;
        DWORD menuId;
        WORD bResInfo;
        /*
         * Variable length NULL terminated WCHAR array
         */
        WCHAR szText;
        /*
         * DWORD dwHelpId, only present when it's a popup menu.
         */
    };
#pragma pack(pop)
}

MenuItemBase::MenuItemBase(const std::wstring& title, const std::wstring& shortcut, bool submenu, MenuBase* parent, DlgBase* dlg) :
    m_dlg(dlg),
    m_title(title),
    m_shortcut(shortcut)
{
    assert(dlg != nullptr);

    auto data = std::make_unique<MenuData>();
    m_data = data.get();
    std::wstring text = title + (shortcut.empty() ? L"" : L"\t" + shortcut);
    data->m_menu.resize(sizeof(MENUEX_TEMPLATE_ITEM) +
                        (text.size() * sizeof(WCHAR)) +
                        (submenu ? sizeof(DWORD) : 0));
    auto item = reinterpret_cast<MENUEX_TEMPLATE_ITEM*>(data->m_menu.data());
    item->dwType = title.empty() ? MFT_SEPARATOR : 0;
    item->menuId = m_dlg->GetNextID();
    item->bResInfo = submenu ? 0x01 : 0x00;

    if (!text.empty())
    {
        wmemcpy(&item->szText, text.data(), text.size());
    }

    if (parent == nullptr)
    {
        m_dlg->m_menu_data.m_children.emplace_back(std::move(data));
    }
    else
    {
        parent->m_data->m_children.emplace_back(std::move(data));
    }
}

void MenuItemBase::SetUnsetStyleBits(DWORD style, SetBits set)
{
    auto item = reinterpret_cast<MENUEX_TEMPLATE_ITEM*>(m_data->m_menu.data());
    if (set == SetBits::Set)
    {
        item->dwState |= style;
    }
    else
    {
        item->dwState &= ~(style);
    }
}

void MenuItemBase::ChangeText(const std::wstring& new_text)
{
    // TODO:
    //auto item = reinterpret_cast<MENUEX_TEMPLATE_ITEM*>(m_data->m_menu.data());
    //m_dlg->SetMenuInfo(item->menuId, /*stuff*/);
}

void MenuItemBase::ChangeTitle(const std::wstring& title)
{
    ChangeText(title + (m_shortcut.empty() ? L"" : L"\t" + m_shortcut));
    m_title = title;
}

void MenuItemBase::ChangeShortcut(const std::wstring& shortcut)
{
    ChangeText(m_title + (shortcut.empty() ? L"" : L"\t" + shortcut));
    m_shortcut = shortcut;
}

void MenuItemBase::RegisterWmCommandHandler(std::function<bool(WORD)>& cb)
{
    auto item = reinterpret_cast<MENUEX_TEMPLATE_ITEM*>(m_data->m_menu.data());
    m_dlg->RegisterWmControlCallback(item->menuId, cb);
}

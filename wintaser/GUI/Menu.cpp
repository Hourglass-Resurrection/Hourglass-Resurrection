/*
 * (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include <Windows.h>

#include <cwchar>
#include <string>
#include <vector>

#include "Menu.h"

/*
 * Reference MENUEX_TEMPLATE_HEADER structure
 * struct MENUEX_TEMPLATE_HEADER
 * {
 *     WORD wVersion;
 *     WORD wOffset;
 *     DWORD dwHelpId;
 * };
 *
 * Reference MENUEX_TEMPLATE_ITEM structure (PopUp menu)
 * struct MENUEX_TEMPLATE_ITEM
 * {
 *     DWORD dwType;
 *     DWORD dwState;
 *     DWORD menuId;
 *     WORD bResInfo;
 *     WCHAR szText[];  <-- Variable length NULL terminated WCHAR array
 *     DWORD dwHelpId;  <-- Not included in the struct when a regular menu item is added
 * };
 *
 * Reference MENUEX_TEMPLATE_ITEM structure (Regular menu-item)
 * struct MENUEX_TEMPLATE_ITEM
 * {
 *     DWORD dwType;
 *     DWORD dwState;
 *     DWORD menuId;
 *     WORD bResInfo;
 *     WCHAR szText[];  <-- Variable length NULL terminated WCHAR array
 * };
 */

namespace
{
    enum : DWORD
    {
        IDC_STATIC = static_cast<DWORD>(-1),
    };
    enum : DWORD
    {
        MENUEX_TEMPLATE_HEADER_SIZE = (sizeof(WORD) * 2) + sizeof(DWORD),
        MENUEX_TEMPLATE_ITEM_BASE_SIZE = (sizeof(DWORD) * 3) +
                                         (sizeof(WORD) * 1) +
                                         (sizeof(WCHAR) * 1),
    };
};

Menu::Menu() :
    m_loaded_menu(nullptr)
{
    DWORD iterator = 0;
    m_menu.resize(MENUEX_TEMPLATE_HEADER_SIZE);
    *reinterpret_cast<LPWORD>(&(m_menu[iterator])) = 0x01;
    iterator += sizeof(WORD);
    *reinterpret_cast<LPWORD>(&(m_menu[iterator])) = 0x04;
}

Menu::~Menu()
{
    if (m_loaded_menu != nullptr)
    {
        DestroyMenu(m_loaded_menu);
    }
}

void Menu::AddMenuCategory(const std::wstring& name, DWORD id, bool enabled, bool last)
{
    DWORD state = (enabled ? MFS_ENABLED : MFS_DISABLED);
    WORD res = (last ? 0x80 : 0x00) | 0x01;
    AddMenuObject(name, id, MFT_STRING, state, res);
}

void Menu::AddSubMenu(const std::wstring& name, DWORD id, bool enabled, bool last)
{
    /*
     * It's the same code-segment, just call the Category function instead
     */
    AddMenuCategory(name, id, enabled, last);
}

void Menu::AddMenuItem(const std::wstring& name, DWORD id, bool enabled, bool last, bool default)
{
    DWORD state = (enabled ? MFS_ENABLED : MFS_DISABLED) | (default ? MFS_DEFAULT : 0);
    WORD res = (last ? 0x80 : 0x00);
    AddMenuObject(name, id, MFT_STRING, state, res);
}

void Menu::AddCheckableMenuItem(const std::wstring& name, DWORD id,
                                bool enabled, bool checked, bool last)
{
    DWORD state = (enabled ? MFS_ENABLED : MFS_DISABLED) |
                  (checked ? MFS_CHECKED : MFS_UNCHECKED);
    WORD res = (last ? 0x80 : 0x00);
    AddMenuObject(name, id, MFT_STRING, state, res);
}

void Menu::AddMenuItemSeparator()
{
    AddMenuObject(L"", IDC_STATIC, MFT_SEPARATOR, 0, 0);
}

void Menu::AddMenuObject(const std::wstring& name, DWORD id, DWORD type, DWORD state, WORD res)
{
    DWORD iterator = m_menu.size();
    DWORD new_size = MENUEX_TEMPLATE_ITEM_BASE_SIZE;
    new_size += (sizeof(WCHAR) * name.size());
    new_size += ((res & 0x01) != 0x00) ? sizeof(DWORD) : 0;
    new_size += iterator;
    new_size += (sizeof(DWORD) - (new_size % sizeof(DWORD))) % sizeof(DWORD);

    m_menu.resize(new_size);

    *reinterpret_cast<LPDWORD>(&(m_menu[iterator])) = type;
    iterator += sizeof(DWORD);
    *reinterpret_cast<LPDWORD>(&(m_menu[iterator])) = state;
    iterator += sizeof(DWORD);
    *reinterpret_cast<LPDWORD>(&(m_menu[iterator])) = id;
    iterator += sizeof(DWORD);
    *reinterpret_cast<LPWORD>(&(m_menu[iterator])) = res;
    iterator += sizeof(WORD);

    if (type != MFT_SEPARATOR)
    {
        wmemcpy(reinterpret_cast<LPWSTR>(&(m_menu[iterator])), name.c_str(), name.size());
    }
}

/*
 * TODO: Make sure attaching a menu fixes the size of the dialog
 * -- Warepire
 */

bool Menu::AttachMenu(HWND window)
{
    m_loaded_menu = LoadMenuIndirectW(reinterpret_cast<LPMENUTEMPLATEW>(m_menu.data()));
    if (m_loaded_menu == nullptr)
    {
        return false;
    }
    if (SetMenu(window, m_loaded_menu) == FALSE)
    {
        DestroyMenu(m_loaded_menu);
        m_loaded_menu = nullptr;
        return false;
    }
    return true;
}

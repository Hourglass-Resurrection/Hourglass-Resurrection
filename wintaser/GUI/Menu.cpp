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
    static const DWORD IDC_STATIC = -1;
    static const DWORD MENUEX_TEMPLATE_HEADER_SIZE = 8;
    static const DWORD MENUEX_TEMPLATE_ITEM_BASE_SIZE = (sizeof(DWORD) * 3) +
                                                        (sizeof(WORD) * 1) +
                                                        (sizeof(WCHAR) * 1);
};

Menu::Menu()
{
    DWORD iterator = 0;
    menu.resize(MENUEX_TEMPLATE_HEADER_SIZE);
    *reinterpret_cast<WORD*>(&(menu[iterator])) = 0x01;
    iterator += sizeof(WORD);
    *reinterpret_cast<WORD*>(&(menu[iterator])) = 0x04;

    loaded_menu = nullptr;
}

Menu::~Menu()
{
    if (loaded_menu != nullptr)
    {
        DestroyMenu(loaded_menu);
    }
}

void Menu::AddMenuCategory(std::wstring name, DWORD id, bool enabled, bool last)
{
    DWORD state = ChooseValue<DWORD>(enabled, MFS_ENABLED, MFS_DISABLED);
    WORD res = ChooseValue<WORD>(last, 0x80, 0x00) | 0x01;
    AddMenuObject(name, id, MFT_STRING, state, res);
}

void Menu::AddSubMenu(std::wstring name, DWORD id, bool enabled, bool last)
{
    /*
     * It's the same code-segment, just call the Category function instead
     */
    AddMenuCategory(name, id, enabled, last);
}

void Menu::AddMenuItem(std::wstring name, DWORD id, bool enabled, bool last, bool default)
{
    DWORD state = ChooseValue<DWORD>(enabled, MFS_ENABLED, MFS_DISABLED);
    state |= ChooseValue<DWORD>(default, MFS_DEFAULT, 0);
    WORD res = ChooseValue<WORD>(last, 0x80, 0x00);
    AddMenuObject(name, id, MFT_STRING, state, res);
}

void Menu::AddMenuItemSeparator()
{
    AddMenuObject(std::wstring(), IDC_STATIC, MFT_SEPARATOR, 0, 0);
}

void Menu::AddMenuObject(std::wstring& name, DWORD id, DWORD type, DWORD state, WORD res)
{
    DWORD iterator = menu.size();
    DWORD new_size = MENUEX_TEMPLATE_ITEM_BASE_SIZE;
    new_size += (sizeof(WCHAR) * name.size());
    new_size += ChooseValue<DWORD>((res & 0x01) != 0x00, sizeof(DWORD), 0);
    new_size += iterator;
    new_size += new_size % sizeof(DWORD);

    menu.resize(new_size);

    *reinterpret_cast<DWORD*>(&(menu[iterator])) = type;
    iterator += sizeof(DWORD);
    *reinterpret_cast<DWORD*>(&(menu[iterator])) = state;
    iterator += sizeof(DWORD);
    *reinterpret_cast<DWORD*>(&(menu[iterator])) = id;
    iterator += sizeof(DWORD);
    *reinterpret_cast<WORD*>(&(menu[iterator])) = res;
    iterator += sizeof(WORD);

    if (type != MFT_SEPARATOR)
    {
        wcscpy(reinterpret_cast<WCHAR*>(&(menu[iterator])), name.c_str());
    }
}

/*
 * TODO: Make sure attaching a menu fixes the size of the dialog
 * -- Warepire
 */

bool Menu::AttachMenu(HWND window)
{
    loaded_menu = LoadMenuIndirectW(reinterpret_cast<MENUTEMPLATEW*>(menu.data()));
    if (loaded_menu == nullptr)
    {
        return false;
    }
    if (SetMenu(window, loaded_menu) == FALSE)
    {
        DestroyMenu(loaded_menu);
        loaded_menu = nullptr;
        return false;
    }
    return true;
}

template<typename T>
__forceinline T Menu::ChooseValue(bool condition, T eval_true, T eval_false)
{
    if (condition)
    {
        return eval_true;
    }
    return eval_false;
}

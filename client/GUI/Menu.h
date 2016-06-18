/*
 * (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#include <Windows.h>

#include <string>
#include <vector>

/*
 * ------------------------------------------------------------------------------------------------
 *                                            USAGE
 * ------------------------------------------------------------------------------------------------
 *
 * Usage is simple. Just remember that everything must happen in sequence and that AttachMenu shall
 * be called from the Dialog Init message.
 */

class Menu
{
public:
    Menu();
    ~Menu();

    void AddMenuCategory(const std::wstring& name, DWORD id, bool enabled, bool last);
    void AddSubMenu(const std::wstring& name, DWORD id, bool enabled, bool last);
    void AddMenuItem(const std::wstring& name, DWORD id, bool enabled, bool last, bool default);
    void AddCheckableMenuItem(const std::wstring& name, DWORD id,
                              bool enabled, bool checked, bool last);
    void AddMenuItemSeparator();

    bool AttachMenu(HWND window);
private:

    void AddMenuObject(const std::wstring& name, DWORD id, DWORD type, DWORD state, WORD res);
    std::vector<BYTE> m_menu;
    HMENU m_loaded_menu;
};

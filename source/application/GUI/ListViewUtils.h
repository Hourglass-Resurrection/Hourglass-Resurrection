/*
 * (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#include <Windows.h>
#include <CommCtrl.h>

#include <map>
#include <set>

/*
 * ListViewUtils helper class
 * --------------------------
 *
 * This class wraps around a report mode ListView to provide an easier-to-use interface for some
 * of the operations that can be performed on it.
 *
 * Besides wrapping the SendMessage() calls to nicer functions this class provides these
 * additional functions:
 * --> Editing of all column items
 * --> Write protection of items per column
 * --> Checkboxes on all items
 *
 * Note that this is not a full wrapper that hides the LVCOLUMN and LVITEM usage.
 */

class ListViewUtils
{
public:
    enum CheckboxStates : INT
    {
        CHECKBOX_STATE_UNCHECKED = 0,
        CHECKBOX_STATE_CHECKED = 1,
    };
    ListViewUtils(HWND list_view_handle, HWND owner_dlg);
    ~ListViewUtils();

    bool EnableEditing();
    bool EnableGridView();
    bool EnableCheckboxes();

    bool AddColumn(INT format, const std::wstring& name, INT width, bool protect);
    /*
     * Add an item to the ListView, an entire row must be completed before starting the next.
     */
    bool AddCheckboxItem(INT column, INT state);
    bool AddTextItem(INT column, const std::wstring& text);

    bool DeleteItem(INT item);

private:
    HWND m_parent;
    HWND m_listview;
    WNDPROC m_listview_previous_cb;

    INT m_num_columns;
    bool m_editing_enabled;
    bool m_checkboxes_enabled;

    std::set<INT> m_protected_columns;
    std::set<INT> m_checkbox_columns;

    HWND m_editcontrol;
    HFONT m_editcontrol_font;
    WNDPROC m_editcontrol_previous_cb;
    INT m_item_being_edited;
    INT m_subitem_being_edited;

    static std::map<HWND, ListViewUtils*> ms_listviewutils_by_handle;
    static LRESULT CALLBACK BaseCallback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    LRESULT ListViewCallback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
};

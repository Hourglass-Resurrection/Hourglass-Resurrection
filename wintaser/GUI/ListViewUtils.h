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
    enum CheckboxStates : int
    {
        CHECKBOX_STATE_UNCHECKED = 0,
        CHECKBOX_STATE_CHECKED = 1,
    };
    ListViewUtils(HWND list_view_handle, HWND owner_dlg);
    ~ListViewUtils();

    bool EnableEditing();
    bool EnableGridView();
    bool EnableCheckboxes();

    bool AddColumn(int format, std::wstring name, int width, bool protect);
    /*
     * Add an item to the ListView, an entire row must be completed before starting the next.
     */
    bool AddCheckboxItem(int column, int state);
    bool AddTextItem(int column, std::wstring text);

    bool DeleteItem(int item);

private:
    HWND listview;
    HWND edit_control;
    HWND parent;
    int num_columns;

    bool enabled_editing;
    bool enabled_checkboxes;

    std::set<int> protected_columns;
    std::set<int> checkbox_columns;

    LRESULT ListViewCallback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    WNDPROC listview_previous_cb;
    WNDPROC editcontrol_prebious_cb;

    int item_being_edited;
    int subitem_being_edited;

    HFONT edit_control_font;

    static std::map<HWND, ListViewUtils*> list_view_utils_by_handle;

    static LRESULT CALLBACK BaseCallback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
};

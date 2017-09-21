/*
 * (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include <Windows.h>
#include <CommCtrl.h>

#include <map>
#include <set>

#include "ListViewUtils.h"

namespace
{
    HIMAGELIST checkboxes = nullptr;
    UINT checkbox_init_ref_counter = 0;
    bool InitCheckBoxImageList()
    {
        if (checkboxes != nullptr)
        {
            return true;
        }

        HDC dc = CreateCompatibleDC(nullptr);
        HBITMAP checkbox_bitmaps;
        RECT checkbox_full_rect = {0, 0, 32, 16};

        if (checkbox_bitmaps = CreateCompatibleBitmap(dc, 32, 16))
        {
            checkboxes = ImageList_Create(16, 16, ILC_COLOR, 2, 1);

            if (checkboxes)
            {
                HBITMAP previous_image = static_cast<HBITMAP>(SelectObject(dc, checkbox_bitmaps));

                RECT checkbox_rect = {1, 1, 16, 16};

                FillRect(dc, &checkbox_full_rect, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));

                DrawFrameControl(dc, &checkbox_rect, DFC_BUTTON, DFCS_BUTTONCHECK | DFCS_FLAT);
                checkbox_rect.left += 16;
                checkbox_rect.right += 16;
                DrawFrameControl(dc,
                                 &checkbox_rect,
                                 DFC_BUTTON,
                                 DFCS_BUTTONCHECK | DFCS_FLAT | DFCS_CHECKED);

                SelectObject(dc, previous_image);

                ImageList_Add(checkboxes, checkbox_bitmaps, 0);

                return true;
            }
        }
        return false;
    }

    enum : UINT
    {
        EDIT_CONTROL_BUFFER_SIZE = 256,
    };
};

std::map<HWND, ListViewUtils*> ListViewUtils::ms_listviewutils_by_handle;

ListViewUtils::ListViewUtils(HWND list_view_handle, HWND owner_dlg)
    : m_parent(owner_dlg)
    , m_listview(list_view_handle)
    , m_listview_previous_cb(nullptr)
    , m_num_columns(0)
    , m_editing_enabled(false)
    , m_checkboxes_enabled(false)
    , m_editcontrol(nullptr)
    , m_editcontrol_font(nullptr)
    , m_editcontrol_previous_cb(nullptr)
    , m_item_being_edited(-1)
    , m_subitem_being_edited(-1)
{
}

ListViewUtils::~ListViewUtils()
{
    if (m_checkboxes_enabled == true)
    {
        checkbox_init_ref_counter--;
    }
    if (checkbox_init_ref_counter == 0 && checkboxes != nullptr)
    {
        ImageList_Destroy(checkboxes);
        checkboxes = nullptr;
    }
}

bool ListViewUtils::EnableEditing()
{
    LONG rv = SetWindowLongW(m_listview,
                             GWL_WNDPROC,
                             reinterpret_cast<LONG>(ListViewUtils::BaseCallback));
    m_listview_previous_cb = reinterpret_cast<WNDPROC>(rv);
    ms_listviewutils_by_handle.insert(std::make_pair(m_listview, this));
    m_editing_enabled = true;
    return true;
}
bool ListViewUtils::EnableGridView()
{
    DWORD ex_styles = SendMessageW(m_listview, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
    SendMessageW(m_listview, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, ex_styles | LVS_EX_GRIDLINES);
    return true;
}
bool ListViewUtils::EnableCheckboxes()
{
    bool ret = InitCheckBoxImageList();
    if (ret == false)
    {
        return false;
    }
    checkbox_init_ref_counter++;
    DWORD ex_styles = SendMessageW(m_listview, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
    SendMessageW(m_listview, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, ex_styles | LVS_EX_SUBITEMIMAGES);
    SendMessageW(m_listview, LVM_SETIMAGELIST, LVSIL_SMALL, reinterpret_cast<LPARAM>(checkboxes));
    return true;
}

bool ListViewUtils::AddColumn(INT format, const std::wstring& name, INT width, bool protect)
{
    LVCOLUMNW column;
    memset(&column, 0, sizeof(column));
    column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    column.fmt = format;
    column.cx = width;
    column.pszText = const_cast<LPWSTR>(name.c_str());
    if (protect == true)
    {
        m_protected_columns.insert(m_num_columns);
    }
    SendMessageW(m_listview, LVM_INSERTCOLUMN, m_num_columns++, reinterpret_cast<LPARAM>(&column));
    return true;
}

bool ListViewUtils::AddCheckboxItem(INT column, INT state)
{
    if (column >= m_num_columns)
    {
        return false;
    }
    LVITEMW item;
    memset(&item, 0, sizeof(item));
    item.mask = LVIF_IMAGE;
    item.iSubItem = column;
    item.iImage = state;
    if (column == 0)
    {
        item.iItem = INT_MAX;
        SendMessageW(m_listview, LVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&item));
    }
    else
    {
        item.iItem = SendMessageW(m_listview, LVM_GETITEMCOUNT, 0, 0) - 1;
        SendMessageW(m_listview, LVM_SETITEM, 0, reinterpret_cast<LPARAM>(&item));
    }
    m_checkbox_columns.insert(column);
    return true;
}
bool ListViewUtils::AddTextItem(INT column, const std::wstring& text)
{
    if (column >= m_num_columns)
    {
        return false;
    }
    LVITEMW item;
    memset(&item, 0, sizeof(item));
    item.mask = LVIF_TEXT;
    item.iSubItem = column;
    item.pszText = const_cast<LPWSTR>(text.c_str());
    /*
     * Due to enabling icons for checkboxes, if the first column is for text then the icon must be
     * specifically set to transparent. Otherwise it will draw the first image in the assigned
     * imagelist as an icon in this column.
     *
     * Use an abnormally large number to ensure the item ends up last when sending the
     * LVM_INSERTITEM message.
     */
    if (column == 0)
    {
        item.mask |= LVIF_IMAGE;
        item.iImage = -1;
        item.iItem = INT_MAX;

        SendMessageW(m_listview, LVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&item));
    }
    else
    {
        item.iItem = SendMessageW(m_listview, LVM_GETITEMCOUNT, 0, 0) - 1;
        SendMessageW(m_listview, LVM_SETITEM, 0, reinterpret_cast<LPARAM>(&item));
    }
    return true;
}

bool ListViewUtils::DeleteItem(INT item)
{
    /*
     * Not yet implemented
     * -- Warepire
     */
    MessageBoxW(nullptr,
                L"ListViewUtils::DeleteItem(int item) is not yet implemented.",
                L"Not yet implemented",
                MB_ICONASTERISK | MB_OK);
    return true;
}

/*
 * TODO:
 * Break out of Listview focus with 'ESC'
 */
LRESULT ListViewUtils::ListViewCallback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (hwnd == m_listview)
    {
        switch (msg)
        {
        case WM_LBUTTONDOWN:
        {
            if (m_editing_enabled == false)
            {
                break;
            }
            if (wparam != MK_LBUTTON)
            {
                return 0;
            }
            if (m_editcontrol != nullptr)
            {
                SendMessageW(m_editcontrol, WM_KILLFOCUS, 0, 0);
            }
            LVHITTESTINFO hittest_info;
            hittest_info.pt.x = static_cast<LONG>(LOWORD(lparam));
            hittest_info.pt.y = static_cast<LONG>(HIWORD(lparam));
            LRESULT rv = SendMessageW(m_listview,
                                      LVM_SUBITEMHITTEST,
                                      0,
                                      reinterpret_cast<LPARAM>(&hittest_info));
            if (rv != -1
                && m_protected_columns.find(hittest_info.iSubItem) == m_protected_columns.end())
            {
                RECT subitem_rect;
                memset(&subitem_rect, 0, sizeof(subitem_rect));
                subitem_rect.top = hittest_info.iSubItem;

                if (m_checkbox_columns.find(hittest_info.iSubItem) != m_checkbox_columns.end())
                {
                    subitem_rect.left = LVIR_ICON;
                    SendMessageW(m_listview,
                                 LVM_GETSUBITEMRECT,
                                 hittest_info.iItem,
                                 reinterpret_cast<LPARAM>(&subitem_rect));

                    if ((hittest_info.pt.x >= subitem_rect.left)
                        && (hittest_info.pt.x <= subitem_rect.right)
                        && (hittest_info.pt.y >= subitem_rect.top)
                        && (hittest_info.pt.y <= subitem_rect.bottom))
                    {
                        LVITEM item;
                        item.mask = LVIF_IMAGE;
                        item.iItem = hittest_info.iItem;
                        item.iSubItem = hittest_info.iSubItem;
                        SendMessageW(m_listview, LVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&item));

                        if (item.iImage == CHECKBOX_STATE_UNCHECKED)
                        {
                            item.iImage = CHECKBOX_STATE_CHECKED;
                        }
                        else
                        {
                            item.iImage = CHECKBOX_STATE_UNCHECKED;
                        }

                        SendMessageW(m_listview, LVM_SETITEMW, 0, reinterpret_cast<LPARAM>(&item));
                    }
                }
                else
                {
                    subitem_rect.left = LVIR_BOUNDS;
                    SendMessageW(m_listview,
                                 LVM_GETSUBITEMRECT,
                                 hittest_info.iItem,
                                 reinterpret_cast<LPARAM>(&subitem_rect));
                    INT w = subitem_rect.right - subitem_rect.left;
                    INT h = subitem_rect.bottom - subitem_rect.top;
                    /*
                     * Calling LVM_GETSUBITEMRECT on column 0 is the same as calling
                     * LVM_GETITEMRECT on the row, so the end of the row is returned inside the
                     * RECT's right member. Get the RECT of column 1 as well, and use the left
                     * member of the temp RECT to provide the correct right member for the
                     * column 0 RECT.
                     */
                    if (hittest_info.iSubItem == 0)
                    {
                        RECT temp;
                        memset(&temp, 0, sizeof(temp));
                        temp.top = 1;
                        temp.left = LVIR_BOUNDS;
                        SendMessageW(m_listview,
                                     LVM_GETSUBITEMRECT,
                                     hittest_info.iItem,
                                     reinterpret_cast<LPARAM>(&temp));
                        subitem_rect.right = temp.left;
                    }

                    WCHAR buf[EDIT_CONTROL_BUFFER_SIZE];
                    LVITEMW item;
                    item.mask = LVIF_TEXT;
                    item.iItem = hittest_info.iItem;
                    item.iSubItem = hittest_info.iSubItem;
                    item.pszText = buf;
                    item.cchTextMax = ARRAYSIZE(buf) - 1;
                    SendMessageW(m_listview, LVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&item));

                    /*
                     * TODO: Something more clever than this magical 8...
                     *       An algorithm for converting from pixels to logical units needed.
                     * -- Warepire
                     */
                    m_editcontrol_font = CreateFontW(-(h - 8),
                                                     0,
                                                     0,
                                                     0,
                                                     FW_NORMAL,
                                                     FALSE,
                                                     FALSE,
                                                     FALSE,
                                                     DEFAULT_CHARSET,
                                                     OUT_DEFAULT_PRECIS,
                                                     CLIP_DEFAULT_PRECIS,
                                                     DEFAULT_QUALITY,
                                                     FF_DONTCARE,
                                                     L"MS Shell Dlg");
                    if (m_editcontrol_font == nullptr)
                    {
                        MessageBoxW(m_parent,
                                    L"Setting font failed!",
                                    L"Error!",
                                    MB_ICONERROR | MB_OK);
                        return 0;
                    }

                    /*
                     * It's very important to set WANTRETURN and MULTILINE, otherwise we cannot
                     * capture when the 'Enter' key is pressed.
                     */
                    DWORD es_style = ES_WANTRETURN | ES_MULTILINE | ES_AUTOHSCROLL;
                    /*
                     * The offsets on position and size are to prevent drawing over the lines in
                     * the ListView.
                     */
                    m_editcontrol = CreateWindowExW(0,
                                                    L"EDIT",
                                                    buf,
                                                    WS_CHILD | WS_VISIBLE | es_style,
                                                    subitem_rect.left + 2,
                                                    subitem_rect.top,
                                                    w - 2,
                                                    h - 1,
                                                    m_listview,
                                                    nullptr,
                                                    GetModuleHandleW(nullptr),
                                                    nullptr);
                    if (m_editcontrol == nullptr)
                    {
                        MessageBoxW(m_parent,
                                    L"Creating edit failed!",
                                    L"Error!",
                                    MB_ICONERROR | MB_OK);
                        /*
                         * TODO: Something more clever here.
                         * -- Warepire
                         */
                        return 0;
                    }

                    SendMessageW(m_editcontrol,
                                 WM_SETFONT,
                                 reinterpret_cast<WPARAM>(m_editcontrol_font),
                                 MAKELPARAM(TRUE, 0));
                    SendMessageW(m_editcontrol, EM_SETSEL, 0xFFFF, 0xFFFF);

                    SetFocus(m_editcontrol);
                    LONG proc = SetWindowLongW(m_editcontrol,
                                               GWL_WNDPROC,
                                               reinterpret_cast<LONG>(ListViewUtils::BaseCallback));
                    m_editcontrol_previous_cb = reinterpret_cast<WNDPROC>(proc);
                    ms_listviewutils_by_handle.insert(std::make_pair(m_editcontrol, this));
                    m_item_being_edited = hittest_info.iItem;
                    m_subitem_being_edited = hittest_info.iSubItem;
                }
            }
            return 0;
        }
        case WM_NCDESTROY:
            /*
             * The ListView is going away, clean up the map, and then let the previous callback
             * deal with the proper cleanup of the ListView.
             */
            ms_listviewutils_by_handle.erase(m_listview);
            break;
        default:
            break;
        }
        return CallWindowProcW(m_listview_previous_cb, hwnd, msg, wparam, lparam);
    }
    else if (hwnd == m_editcontrol)
    {
        switch (msg)
        {
        case WM_KEYDOWN:
        {
            switch (wparam)
            {
            case VK_RETURN:
            case VK_TAB:
            {
                /*
                     * Since this control gets re-created all the time the WM_KEYUP message
                     * does not go through this control every time leading to state conflicts
                     * using the WM_KEYDOWN message to detect shift-holding.
                     * Using GetKeyState gets around that issue.
                     */
                bool shift_held = (GetKeyState(VK_SHIFT) & 0x8000) == 0x8000;
                /*
                     * Calling LVM_GETSUBITEMRECT on invalid items doesn't cause SendMessageW
                     * to return an error, the resulting RECT will however be filled with
                     * obscenely large values instead. Mitigate this by finding out the length
                     * of the entire row and use that as a comparison.
                     */
                RECT bounds;
                memset(&bounds, 0, sizeof(bounds));
                bounds.left = LVIR_BOUNDS;
                SendMessageW(m_listview, LVM_GETITEMRECT, 0, reinterpret_cast<LPARAM>(&bounds));

                INT num_rows = SendMessageW(m_listview, LVM_GETITEMCOUNT, 0, 0) - 1;

                RECT subitem_rect;
                INT next_item = m_item_being_edited;
                INT next_subitem = m_subitem_being_edited;

                /*
                     * Using a do-while loop for a nicer way to do this item scanning without
                     * the use of goto.
                     */
                do
                {
                    memset(&subitem_rect, 0, sizeof(subitem_rect));
                    do
                    {
                        if (shift_held && wparam == VK_TAB)
                        {
                            next_subitem--;
                        }
                        else
                        {
                            next_subitem++;
                        }
                    } while (
                        (m_protected_columns.find(next_subitem) != m_protected_columns.end())
                        || (m_checkbox_columns.find(next_subitem) != m_checkbox_columns.end()));

                    subitem_rect.top = next_subitem;
                    subitem_rect.left = LVIR_BOUNDS;
                    LRESULT rv = SendMessageW(m_listview,
                                              LVM_GETSUBITEMRECT,
                                              next_item,
                                              reinterpret_cast<LPARAM>(&subitem_rect));

                    if (static_cast<UINT>(subitem_rect.left) > static_cast<UINT>(bounds.right)
                        || static_cast<UINT>(subitem_rect.right) > static_cast<UINT>(bounds.right))
                    {
                        if (shift_held && wparam == VK_TAB)
                        {
                            if (next_item == 0)
                            {
                                SetFocus(m_parent);
                                return 0;
                            }
                            next_item--;
                            next_subitem = m_num_columns;
                        }
                        else
                        {
                            if (next_item == num_rows)
                            {
                                SetFocus(m_parent);
                                return 0;
                            }
                            next_item++;
                            next_subitem = -1;
                        }
                        continue;
                    }
                    break;
                } while (true);

                SendMessageW(m_editcontrol, WM_KILLFOCUS, 0, 0);

                /*
                     * Just dump the message the in the message queue, don't block for a
                     * return value, otherwise in larger lists we will have dangerously deep
                     * rectursion in this message handler.
                     */
                PostMessageW(m_listview,
                             WM_LBUTTONDOWN,
                             MK_LBUTTON,
                             MAKELPARAM(subitem_rect.left + 1, subitem_rect.top + 1));
                return 0;
            }
            default:
                break;
            }
            break;
        }
        case WM_KILLFOCUS:
        {
            WCHAR new_text[EDIT_CONTROL_BUFFER_SIZE];
            GetWindowTextW(m_editcontrol, new_text, ARRAYSIZE(new_text) - 1);

            LVITEMW item;
            memset(&item, 0, sizeof(item));
            item.mask = LVIF_TEXT;
            item.iItem = m_item_being_edited;
            item.iSubItem = m_subitem_being_edited;
            item.pszText = new_text;
            SendMessageW(m_listview, LVM_SETITEMTEXTW, item.iItem, reinterpret_cast<LPARAM>(&item));

            DestroyWindow(m_editcontrol);
            break;
        }
        case WM_NCDESTROY:
            ms_listviewutils_by_handle.erase(m_editcontrol);
            m_editcontrol = nullptr;
            DeleteObject(m_editcontrol_font);
            m_editcontrol_font = nullptr;
            break;
        default:
            break;
        }
        return CallWindowProcW(m_editcontrol_previous_cb, hwnd, msg, wparam, lparam);
    }
    /*
     * Technically this path is impossible, but safety first.
     */
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

LRESULT CALLBACK ListViewUtils::BaseCallback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    /*
     * Very simple BaseCallback, with a very simple sanity check.
     * During the window destruction the mapping is erased, so call the default proc
     * if we can't find the mapping.
     * Let the member callback do all the work.
     * The member callback isn't even necessary, but it makes accessing the internals of
     * ListViewUtils much nicer.
     */
    auto listviewutils_it = ms_listviewutils_by_handle.find(hwnd);
    if (listviewutils_it != ms_listviewutils_by_handle.end())
    {
        return listviewutils_it->second->ListViewCallback(hwnd, msg, wparam, lparam);
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

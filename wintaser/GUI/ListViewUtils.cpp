#include <Windows.h>
#include <CommCtrl.h>

#include <map>
#include <set>

#include "ListViewUtils.h"

namespace
{
    HIMAGELIST checkboxes = nullptr;
    unsigned int checkbox_init_ref_counter = 0;
    bool InitCheckBoxImageList()
    {
        if (checkboxes != nullptr)
        {
            return true;
        }

        HDC dc = CreateCompatibleDC(NULL);
        HBITMAP checkbox_bitmaps;
        RECT checkbox_full_rect = { 0, 0, 32, 16 };

        if (checkbox_bitmaps = CreateCompatibleBitmap(dc, 32, 16))
        {
            checkboxes = ImageList_Create(16, 16, ILC_COLOR, 2, 1);
        
            if (checkboxes)
            {
                HBITMAP previous_image = static_cast<HBITMAP>(SelectObject(dc, checkbox_bitmaps));

                RECT checkbox_rect = { 1, 1, 16, 16 };

                FillRect(dc, &checkbox_full_rect, reinterpret_cast<HBRUSH>(COLOR_WINDOW+1));
                
                DrawFrameControl(dc,
                                 &checkbox_rect,
                                 DFC_BUTTON, DFCS_BUTTONCHECK | DFCS_FLAT);
                checkbox_rect.left  += 16;
                checkbox_rect.right += 16;
                DrawFrameControl(dc,
                                 &checkbox_rect,
                                 DFC_BUTTON, DFCS_BUTTONCHECK | DFCS_FLAT | DFCS_CHECKED);
                
                SelectObject(dc, previous_image);

                ImageList_Add(checkboxes, checkbox_bitmaps, 0);

                return true;
            }
        }
        return false;
    }

    static const unsigned int EDIT_CONTROL_BUFFER_SIZE = 256;
};

std::map<HWND, ListViewUtils*> ListViewUtils::list_view_utils_by_handle;

ListViewUtils::ListViewUtils(HWND list_view_handle, HWND owner_dlg) :
        listview(list_view_handle),
        edit_control(nullptr),
        parent(owner_dlg),
        num_columns(0),
        enabled_editing(false),
        enabled_checkboxes(false),
        listview_previous_cb(nullptr),
        editcontrol_prebious_cb(nullptr),
        item_being_edited(-1),
        subitem_being_edited(-1),
        edit_control_font(nullptr)
{
}

ListViewUtils::~ListViewUtils()
{
    if (enabled_checkboxes == true)
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
    LONG rv = SetWindowLongW(listview,
                             GWL_WNDPROC,
                             reinterpret_cast<LONG>(ListViewUtils::BaseCallback));
    listview_previous_cb = reinterpret_cast<WNDPROC>(rv);
    list_view_utils_by_handle.insert(std::make_pair(listview, this));
    enabled_editing = true;
    return true;
}
bool ListViewUtils::EnableGridView()
{
    DWORD ex_styles = SendMessageW(listview, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
    SendMessageW(listview, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, ex_styles | LVS_EX_GRIDLINES);
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
    DWORD ex_styles = SendMessageW(listview, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
    SendMessageW(listview, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, ex_styles | LVS_EX_SUBITEMIMAGES);
    SendMessageW(listview,
                 LVM_SETIMAGELIST,
                 LVSIL_SMALL,
                 reinterpret_cast<LPARAM>(checkboxes));
    return true;
}

bool ListViewUtils::AddColumn(int format, std::wstring name, int width, bool protect)
{
    LVCOLUMNW column;
    memset(&column, 0, sizeof(column));
    column.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
    column.fmt = format;
    column.cx = width;
    column.pszText = const_cast<LPWSTR>(name.c_str());
    if (protect == true)
    {
        protected_columns.insert(num_columns);
    }
    SendMessageW(listview, LVM_INSERTCOLUMN, num_columns++, reinterpret_cast<LPARAM>(&column));
    return true;
}

bool ListViewUtils::AddCheckboxItem(int column, int state)
{
    if (column >= num_columns)
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
        SendMessageW(listview, LVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&item));
    }
    else
    {
        item.iItem = SendMessageW(listview, LVM_GETITEMCOUNT, 0, 0) - 1;
        SendMessageW(listview, LVM_SETITEM, 0, reinterpret_cast<LPARAM>(&item));
    }
    checkbox_columns.insert(column);
    return true;
}
bool ListViewUtils::AddTextItem(int column, std::wstring text)
{
    if (column >= num_columns)
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

        SendMessageW(listview, LVM_INSERTITEM, 0, reinterpret_cast<LPARAM>(&item));
    }
    else
    {
        item.iItem = SendMessageW(listview, LVM_GETITEMCOUNT, 0, 0) - 1;
        SendMessageW(listview, LVM_SETITEM, 0, reinterpret_cast<LPARAM>(&item));
    }
    return true;
}

bool ListViewUtils::DeleteItem(int item)
{
    // Not yet implemented
    return true;
}

/*
 * TODO:
 * Break out of Listview focus with 'ESC'
 */
LRESULT ListViewUtils::ListViewCallback(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (hwnd == listview)
    {
        switch (msg)
        {
            case WM_LBUTTONDOWN:
            {
                if (enabled_editing == false)
                {
                    break;
                }
                if (wparam != MK_LBUTTON)
                {
                    return 0;
                }
                if (edit_control != nullptr)
                {
                    SendMessageW(edit_control, WM_KILLFOCUS, 0, 0);
                }
                LVHITTESTINFO hittest_info;
                hittest_info.pt.x = static_cast<LONG>(LOWORD(lparam));
                hittest_info.pt.y = static_cast<LONG>(HIWORD(lparam));
                LRESULT rv = SendMessageW(listview,
                                          LVM_SUBITEMHITTEST,
                                          0,
                                          reinterpret_cast<LPARAM>(&hittest_info));
                if (rv != -1 &&
                    protected_columns.find(hittest_info.iSubItem) == protected_columns.end())
                {
                    RECT subitem_rect;
                    memset(&subitem_rect, 0, sizeof(subitem_rect));
                    subitem_rect.top = hittest_info.iSubItem;

                    if (checkbox_columns.find(hittest_info.iSubItem) != checkbox_columns.end())
                    {
                        subitem_rect.left = LVIR_ICON;
                        SendMessageW(listview,
                                     LVM_GETSUBITEMRECT,
                                     hittest_info.iItem,
                                     reinterpret_cast<LPARAM>(&subitem_rect));

                        if ((hittest_info.pt.x >= subitem_rect.left) &&
                            (hittest_info.pt.x <= subitem_rect.right) &&
                            (hittest_info.pt.y >= subitem_rect.top) &&
                            (hittest_info.pt.y <= subitem_rect.bottom))
                        {
                            LVITEM item;
                            item.mask = LVIF_IMAGE;
                            item.iItem = hittest_info.iItem;
                            item.iSubItem = hittest_info.iSubItem;
                            SendMessageW(listview, LVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&item));

                            if (item.iImage == CHECKBOX_STATE_UNCHECKED)
                            {
                                item.iImage = CHECKBOX_STATE_CHECKED;
                            }
                            else
                            {
                                item.iImage = CHECKBOX_STATE_UNCHECKED;
                            }

                            SendMessageW(listview, LVM_SETITEMW, 0, reinterpret_cast<LPARAM>(&item));
                        }
                    }
                    else
                    {
                        subitem_rect.left = LVIR_BOUNDS;
                        SendMessageW(listview,
                                     LVM_GETSUBITEMRECT,
                                     hittest_info.iItem,
                                     reinterpret_cast<LPARAM>(&subitem_rect));
                        int w = subitem_rect.right - subitem_rect.left;
                        int h = subitem_rect.bottom - subitem_rect.top;
                        /*
                         * Calling LVM_GETSUBITEMRECT on column 0 is the same as calling
                         * LVM_GETITEMRECT on the row, so the end of the row is returned inside the
                         * RECT's right member. Get the RECT of column 1 as well, and use the left
                         * member of the temp RECT to provide the correct right member for the column 0
                         * RECT.
                         */
                        if (hittest_info.iSubItem == 0)
                        {
                            RECT temp;
                            memset(&temp, 0, sizeof(temp));
                            temp.top = 1;
                            temp.left = LVIR_BOUNDS;
                            SendMessageW(listview,
                                         LVM_GETSUBITEMRECT,
                                         hittest_info.iItem,
                                         reinterpret_cast<LPARAM>(&temp));
                            subitem_rect.right = temp.left;
                        }

                        wchar_t buf[EDIT_CONTROL_BUFFER_SIZE];
                        LVITEM item;
                        item.mask = LVIF_TEXT;
                        item.iItem = hittest_info.iItem;
                        item.iSubItem = hittest_info.iSubItem;
                        item.pszText = buf;
                        item.cchTextMax = ARRAYSIZE(buf) - 1;
                        SendMessageW(listview, LVM_GETITEMW, 0, reinterpret_cast<LPARAM>(&item));

                        /*
                         * TODO: Something more clever than this magical 8...
                         *       An algorithm for converting from pixels to logical units needed.
                         * -- Warepire
                         */
                        edit_control_font = CreateFontW(-(h - 8),
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
                        if (edit_control_font == nullptr)
                        {
                            MessageBoxW(parent, L"Setting font failed!", L"Error!", MB_ICONERROR | MB_OK);
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
                        edit_control = CreateWindowExW(0,
                                                       L"EDIT",
                                                       buf, 
                                                       WS_CHILD | WS_VISIBLE | es_style,
                                                       subitem_rect.left + 2,
                                                       subitem_rect.top,
                                                       w - 2,
                                                       h - 1,
                                                       listview,
                                                       nullptr,
                                                       GetModuleHandleW(nullptr),
                                                       nullptr);
                        if (edit_control == nullptr)
                        {
                            MessageBoxW(parent, L"Creating edit failed!", L"Error!", MB_ICONERROR | MB_OK);
                            /*
                             * TODO: Something more clever here.
                             * -- Warepire
                             */
                            return 0;
                        }

                        SendMessageW(edit_control,
                                     WM_SETFONT,
                                     reinterpret_cast<WPARAM>(edit_control_font),
                                     MAKELPARAM(TRUE,0));
                        SendMessageW(edit_control, EM_SETSEL, 0xFFFF, 0xFFFF);

                        SetFocus(edit_control);
                        LONG proc =
                            SetWindowLongW(edit_control,
                                           GWL_WNDPROC,
                                           reinterpret_cast<LONG>(ListViewUtils::BaseCallback));
                        editcontrol_prebious_cb = reinterpret_cast<WNDPROC>(proc);
                        list_view_utils_by_handle.insert(std::make_pair(edit_control, this));
                        item_being_edited = hittest_info.iItem;
                        subitem_being_edited = hittest_info.iSubItem;
                    }
                }
                return 0;
            }
            case WM_NCDESTROY:
                /*
                 * The ListView is going away, clean up the map, and then let the previous callback
                 * deal with the proper cleanup of the ListView.
                 */
                list_view_utils_by_handle.erase(listview);
                break;
            default:
                break;
        }
        return CallWindowProcW(listview_previous_cb, hwnd, msg, wparam, lparam);
    }
    else if (hwnd == edit_control)
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
                        SendMessageW(listview,
                                     LVM_GETITEMRECT,
                                     0,
                                     reinterpret_cast<LPARAM>(&bounds));

                        int num_rows = SendMessageW(listview, LVM_GETITEMCOUNT, 0, 0) - 1;

                        RECT subitem_rect;
                        int next_item = item_being_edited;
                        int next_subitem = subitem_being_edited;
                        
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
                            } while ((protected_columns.find(next_subitem) !=
                                      protected_columns.end()) ||
                                     (checkbox_columns.find(next_subitem) != 
                                      checkbox_columns.end()));

                            
                            subitem_rect.top = next_subitem;
                            subitem_rect.left = LVIR_BOUNDS;
                            LRESULT rv = SendMessageW(listview,
                                                      LVM_GETSUBITEMRECT,
                                                      next_item,
                                                      reinterpret_cast<LPARAM>(&subitem_rect));

                            if (static_cast<unsigned int>(subitem_rect.left) >
                                    static_cast<unsigned int>(bounds.right) ||
                                static_cast<unsigned int>(subitem_rect.right) >
                                    static_cast<unsigned int>(bounds.right))
                            {
                                if (shift_held && wparam == VK_TAB)
                                {
                                    if (next_item == 0)
                                    {
                                        SetFocus(parent);
                                        return 0;
                                    }
                                    next_item--;
                                    next_subitem = num_columns;
                                }
                                else
                                {
                                    if (next_item == num_rows)
                                    {
                                        SetFocus(parent);
                                        return 0;
                                    }
                                    next_item++;
                                    next_subitem = -1;
                                }
                                continue;
                            }
                            break;
                        } while (true);

                        SendMessageW(edit_control, WM_KILLFOCUS, 0, 0);

                        /*
                         * Just dump the message the in the message queue, don't block for a
                         * return value, otherwise in larger lists we will have dangerously deep
                         * rectursion in this message handler.
                         */
                        PostMessageW(listview,
                                     WM_LBUTTONDOWN,
                                     MK_LBUTTON,
                                     MAKELPARAM(subitem_rect.left + 1,
                                                subitem_rect.top + 1));
                        return 0;
                    }
                    default:
                        break;
                }
                break;
            }
            case WM_KILLFOCUS:
            {
                wchar_t new_text[EDIT_CONTROL_BUFFER_SIZE];
                GetWindowTextW(edit_control, new_text, ARRAYSIZE(new_text) - 1);

                LVITEMW item;
                memset(&item, 0, sizeof(item));
                item.mask = LVIF_TEXT;
                item.iItem = item_being_edited;
                item.iSubItem = subitem_being_edited;
                item.pszText = new_text;
                SendMessageW(listview,
                             LVM_SETITEMTEXTW,
                             item.iItem,
                             reinterpret_cast<LPARAM>(&item));

                DestroyWindow(edit_control);
                break;
            }
            case WM_NCDESTROY:
                list_view_utils_by_handle.erase(edit_control);
                edit_control = nullptr;
                DeleteObject(edit_control_font);
                edit_control_font = nullptr;
                break;
            default:
                break;
        }
        return CallWindowProcW(editcontrol_prebious_cb, hwnd, msg, wparam, lparam);
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
    auto listviewutils_it = list_view_utils_by_handle.find(hwnd);
    if (listviewutils_it != list_view_utils_by_handle.end())
    {
        return listviewutils_it->second->ListViewCallback(hwnd, msg, wparam, lparam);
    }
    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

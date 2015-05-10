/*
 * (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include <Windows.h>
#include <CommCtrl.h>

#include <cwchar>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "DlgBase.h"

/*
 * Reference DLGTEMPLATEEX structure:
 * struct DLGTEMPLATEEX
 * {
 *     WORD      dlgVer;
 *     WORD      signature;
 *     DWORD     helpID;
 *     DWORD     exStyle;
 *     DWORD     style;
 *     WORD      cDlgItems;            --> Caps out at 0xFFFF items.
 *     short     x;
 *     short     y;
 *     short     cx;
 *     short     cy;
 *     sz_Or_Ord menu;                 --> Does not accept a HMENU, Use "SetMenu"
 *     sz_Or_Ord windowClass;
 *     WCHAR     title[titleLen];
 *     WORD      pointsize;
 *     WORD      weight;
 *     BYTE      italic;
 *     BYTE      charset;
 *     WCHAR     typeface[stringLen];  --> "MS Shell Dlg" pre-defined
 * };
 *
 * Reference DLGITEMTEMPLATEEX structure:
 * struct DLGITEMTEMPLATEEX
 * {
 *     DWORD     helpID;
 *     DWORD     exStyle;
 *     DWORD     style;
 *     short     x;
 *     short     y;
 *     short     cx;
 *     short     cy;
 *     DWORD     id;
 *     sz_Or_Ord windowClass;
 *     sz_Or_Ord title[titleLen];
 *     WORD      extraCount;
 * };
 */

namespace
{
    /*
     * The title must always contain at least a NULL character.
     * The typeface is the string L"MS Shell Dlg\0".
     */
    static const DWORD DLGTEMPLATEEX_BASE_SIZE = (sizeof(WORD) * 2) +
                                                 (sizeof(DWORD) * 3) +
                                                 (sizeof(WORD) * 1) +
                                                 (sizeof(short) * 4) +
                                                 (sizeof(WORD) * 1) +
                                                 (sizeof(WORD) * 1) +
                                                 (sizeof(WCHAR) * 1) +
                                                 (sizeof(WORD) * 2) +
                                                 (sizeof(BYTE) * 2) +
                                                 (sizeof(WCHAR) * 13);

    static const DWORD CDLGITEMS_OFFSET =  (sizeof(WORD) * 2) +
                                           (sizeof(DWORD) * 3);

    /*
     * The windowClass will be evaluated during object construction.
     * The title must always contain at least a NULL character.
     */
    static const DWORD DLGITEMTEMPLATEEX_BASE_SIZE = (sizeof(DWORD) * 3) +
                                                     (sizeof(short) * 4) +
                                                     (sizeof(DWORD) * 1) +
                                                     (sizeof(WCHAR) * 0) +
                                                     (sizeof(WCHAR) * 1) +
                                                     (sizeof(WORD) * 1);
};

DlgBase::DlgBase(std::wstring caption, SHORT x, SHORT y, SHORT w, SHORT h)
{
    DWORD iterator = 0;
    std::vector<BYTE>::size_type struct_size = DLGTEMPLATEEX_BASE_SIZE;

    /*
     * Add the size of the caption
     */
    struct_size += (sizeof(WCHAR) * caption.size());

    /*
     * Make sure the size is DWORD aligned to put the first child objects at the right offset.
     */
    struct_size += struct_size % sizeof(DWORD);

    window.resize(struct_size);

    /*
     * Required value
     */
    *reinterpret_cast<WORD*>(&(window[iterator])) = 0x0001;
    iterator += sizeof(WORD);

    /*
     * EX-type DLGTEMPLATE
     */
    *reinterpret_cast<WORD*>(&(window[iterator])) = 0xFFFF;
    iterator += sizeof(WORD);
    /*
     * Leave HelpID and exStyle alone
     */
    iterator += (sizeof(DWORD) * 2);

    *reinterpret_cast<DWORD*>(&(window[iterator])) = DS_SETFONT | DS_FIXEDSYS |
                                                     DS_MODALFRAME | WS_POPUP |
                                                     WS_CAPTION | WS_SYSMENU |
                                                     WS_MINIMIZEBOX;
    iterator += sizeof(DWORD);

    /*
     * We'll set cDlgItems incrementally with the children.
     */
    iterator += sizeof(WORD);

    *reinterpret_cast<short*>(&(window[iterator])) = x;
    iterator += sizeof(short);
    *reinterpret_cast<short*>(&(window[iterator])) = y;
    iterator += sizeof(short);
    *reinterpret_cast<short*>(&(window[iterator])) = w;
    iterator += sizeof(short);
    *reinterpret_cast<short*>(&(window[iterator])) = h;
    iterator += sizeof(short);

    /*
     * Leave menu and windowClass alone
     */
    iterator += (sizeof(WORD) * 2);

    if(caption.empty() == false)
    {
        wcscpy(reinterpret_cast<WCHAR*>(&(window[iterator])), caption.c_str());
        iterator += (sizeof(WCHAR) * caption.size());
    }
    iterator += sizeof(WCHAR);

    /*
     * pointsize 8
     */
    *reinterpret_cast<WORD*>(&(window[iterator])) = 0x0008;
    iterator += sizeof(WORD);

    *reinterpret_cast<WORD*>(&(window[iterator])) = static_cast<WORD>(FW_NORMAL);
    iterator += sizeof(WORD);
    window[iterator] = static_cast<BYTE>(FALSE);
    iterator += sizeof(BYTE);
    window[iterator] = 0x01;
    iterator += sizeof(BYTE);

    wcscpy(reinterpret_cast<WCHAR*>(&(window[iterator])), L"MS Shell Dlg");

    active = false;
}

DlgBase::~DlgBase()
{
}

void DlgBase::AddPushButton(std::wstring caption,
                            DWORD id,
                            SHORT x, SHORT y,
                            SHORT w, SHORT h,
                            bool default)
{
    AddObject(0,
              WS_GROUP | WS_TABSTOP | (default ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON),
              L"\xFFFF\x0080",
              caption,
              id,
              x, y,
              w, h);
}

void DlgBase::AddCheckbox(std::wstring caption,
                          DWORD id,
                          SHORT x, SHORT y,
                          SHORT w, SHORT h,
                          bool right_hand)
{
    AddObject(0,
              WS_GROUP | WS_TABSTOP | BS_AUTOCHECKBOX | (right_hand ? BS_RIGHTBUTTON : 0),
              L"\xFFFF\x0080",
              caption,
              id,
              x, y,
              w, h);
}

void DlgBase::AddRadioButton(std::wstring caption,
                             DWORD id,
                             SHORT x, SHORT y,
                             SHORT w, SHORT h,
                             bool right_hand,
                             bool group_with_prev)
{
    DWORD style = WS_TABSTOP | BS_AUTORADIOBUTTON;
    AddObject(0,
              style | (group_with_prev ? 0 : WS_GROUP) | (right_hand ? BS_RIGHTBUTTON : 0),
              L"\xFFFF\x0080",
              caption,
              id,
              x, y,
              w, h);
}

void DlgBase::AddEditControl(DWORD id, SHORT x, SHORT y, SHORT w, SHORT h, bool multi_line)
{
    DWORD multi_line_style;
    multi_line_style = (multi_line ? WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN : 0);

    AddObject(0,
              WS_GROUP | WS_BORDER | WS_TABSTOP | multi_line_style,
              L"\xFFFF\x0081",
              std::wstring(),
              id,
              x, y,
              w, h);
}

void DlgBase::AddStaticText(std::wstring caption, DWORD id, SHORT x, SHORT y, SHORT w, SHORT h)
{
    AddObject(0, SS_LEFT, L"\xFFFF\x0082", caption, id, x, y, w, h);
}

void DlgBase::AddStaticPanel(DWORD id, SHORT x, SHORT y, SHORT w, SHORT h)
{
    AddObject(0, SS_GRAYRECT, L"\xFFFF\x0082", std::wstring(), id, x, y, w, h);
}

void DlgBase::AddGroupBox(std::wstring caption, DWORD id, SHORT x, SHORT y, SHORT w, SHORT h)
{
    AddObject(WS_EX_TRANSPARENT, BS_GROUPBOX, L"\xFFFF\x0080", caption, id, x, y, w, h);
}

void DlgBase::AddDropDownList(DWORD id, SHORT x, SHORT y, SHORT w, SHORT drop_distance)
{
    AddObject(0,
              WS_GROUP | WS_VSCROLL | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_HASSTRINGS,
              L"\xFFFF\x0085",
              std::wstring(),
              id,
              x, y,
              w,
              drop_distance);
}

void DlgBase::AddListView(DWORD id,
                          SHORT x, SHORT y,
                          SHORT w, SHORT h,
                          bool editable, bool single_selection)
{
    DWORD default_style = WS_GROUP | WS_BORDER | WS_TABSTOP |
                          LVS_REPORT | LVS_ALIGNLEFT | LVS_SHAREIMAGELISTS;
    AddObject(0,
              default_style | (editable ? LVS_EDITLABELS : 0) | (single_selection ? LVS_SINGLESEL : 0),
              L"SysListView32",
              std::wstring(),
              id,
              x, y,
              w, h);
}

void DlgBase::AddObject(DWORD ex_style, DWORD style,
                        std::wstring window_class,
                        std::wstring& caption,
                        DWORD id,
                        SHORT x, SHORT y,
                        SHORT w, SHORT h)
{
    DWORD iterator = window.size();
    std::vector<BYTE>::size_type new_size = DLGITEMTEMPLATEEX_BASE_SIZE;
    new_size += (sizeof(WCHAR) * window_class.size());
    /*
     * Non-atomic class, also add space for the NULL.
     */
    if (window_class[0] != L'\xFFFF')
    {
        new_size += sizeof(WCHAR);
    }

    new_size += (sizeof(WCHAR) * caption.size());
    new_size += iterator;
    new_size += new_size % sizeof(DWORD);

    window.resize(new_size);

    /*
     * Leave helpID alone
     */
    iterator += sizeof(DWORD);

    *reinterpret_cast<DWORD*>(&(window[iterator])) = ex_style;
    iterator += sizeof(DWORD);
    *reinterpret_cast<DWORD*>(&(window[iterator])) = WS_CHILD | WS_VISIBLE | style;
    iterator += sizeof(DWORD);

    *reinterpret_cast<short*>(&(window[iterator])) = x;
    iterator += sizeof(short);
    *reinterpret_cast<short*>(&(window[iterator])) = y;
    iterator += sizeof(short);
    *reinterpret_cast<short*>(&(window[iterator])) = w;
    iterator += sizeof(short);
    *reinterpret_cast<short*>(&(window[iterator])) = h;
    iterator += sizeof(short);

    *reinterpret_cast<DWORD*>(&(window[iterator])) = id;
    iterator += sizeof(DWORD);

    wcsncpy(reinterpret_cast<WCHAR*>(&(window[iterator])), window_class.c_str(), window_class.size());
    iterator += (sizeof(WCHAR) * window_class.size());
    if (window_class[0] != L'\xFFFF')
    {
        iterator += sizeof(WCHAR);
    }

    if (caption.empty() == false)
    {
        wcscpy(reinterpret_cast<WCHAR*>(&(window[iterator])), caption.c_str());
        iterator += (sizeof(WCHAR) * caption.size());
    }
    iterator += sizeof(WCHAR);

    *reinterpret_cast<WORD*>(&(window[CDLGITEMS_OFFSET])) += 1;
}

INT_PTR DlgBase::SpawnDialogBox(HINSTANCE instance,
                                HWND parent,
                                DlgProcCallback callback,
                                LPARAM init_param,
                                DlgMode mode,
                                DlgProcIndirectMsgLoop msg_loop)
{
    HWND handle;
    INT_PTR result;
    LParamData l_param;

    /*
     * Make sure we don't succeed in running this twice
     */
    if (active)
    {
        SetLastError(ERROR_CAN_NOT_COMPLETE);
        return -1;
    }
    active = true;

    l_param.original_data = init_param;
    l_param.callback = callback;

    switch (mode)
    {
    case PROMPT:
        result = DialogBoxIndirectParamW(instance,
                                         reinterpret_cast<LPCDLGTEMPLATEA>(window.data()),
                                         parent,
                                         reinterpret_cast<DLGPROC>(DlgBase::BaseCallback),
                                         reinterpret_cast<LPARAM>(&l_param));
        break;
    case INDIRECT:
        /*
         * Without a message loop we're doomed, so do the cleanest thing we can.
         */
        if (msg_loop == nullptr)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            active = false;
            return -1;
        }

        handle = CreateDialogIndirectParamW(instance,
                                            reinterpret_cast<LPCDLGTEMPLATEA>(window.data()),
                                            parent,
                                            reinterpret_cast<DLGPROC>(DlgBase::BaseCallback),
                                            reinterpret_cast<LPARAM>(&l_param));
        if (handle == nullptr)
        {
            active = false;
            return -1;
        }

        result = static_cast<INT_PTR>(msg_loop());
    }
    active = false;
    return result;
}

INT_PTR CALLBACK DlgBase::BaseCallback(HWND window, UINT msg, WPARAM w_param, LPARAM l_param)
{
    static std::map<HWND, DlgProcCallback> callback_map;
    switch (msg)
    {
    case WM_INITDIALOG:
        /*
         * Map the callback to the HWND. Then re-use the LPARAM to send what the owner wanted to
         * pass to the callback.
         */
        callback_map[window] = (reinterpret_cast<LParamData*>(l_param))->callback;
        l_param = (reinterpret_cast<LParamData*>(l_param))->original_data;
        break;
    case WM_DESTROY:
        /*
         * Special case, so that we don't just allocate more and more memory.
         * Drop the callback BEFORE calling it, as the callback may call ExitProcess etc.
         */
        DlgProcCallback callback = callback_map[window];
        callback_map.erase(window);
        return callback(window, msg, w_param, l_param);
    }
    /*
     * Every "sub-window" created (i.e. objects) will also come through here,
     * in that case, just return FALSE, saying we didn't handle the message.
     * TODO: Breaks editable ListView?
     */
    try
    {
        return callback_map.at(window)(window, msg, w_param, l_param);
    }
    catch (std::out_of_range&)
    {
        return FALSE;
    }
}

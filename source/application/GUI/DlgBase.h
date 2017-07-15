/*
 * (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#include <Windows.h>
#include <CommCtrl.h>

#include <functional>
#include <map>
#include <string>
#include <vector>

/*
 * A lot of object types are missing, please add them as necessary.
 * Refer to the currently implemented objects when creating new ones.
 * -- Warepire
 */

/*
 * ------------------------------------------------------------------------------------------------
 *                                   Implementation notes
 * ------------------------------------------------------------------------------------------------
 *
 * Usage is simple. Just inherit DlgBase and remember that everything is in DialogPoints instead of
 * pixels.
 *
 * This construction allows for enum IDs for the object IDs, and it is encouraged to use this
 * method to declare them in order to avoid ID collisions. It is also encouraged to keep the IDs
 * private to the class inheriting DlgBase.
 *
 * When creating an Indirect dialog, SpawnDialogBox() will return -1 on creation failure and 0 on
 * success. When creating a Prompt dialog, SpawnDialogBox() will return the result of the dialog
 * box when it exits.
 *
 * The purpose of SendMessage and SendDlgItemMessage is to send messages to the window and it's
 * controls, without keeping track of the HWND.
 * Don't just proxy this call in your window class, wrap it to obscure the message IDs as well.
 * You're meant to have class members that look like this (example code, do not actually use):
 * bool SendCloseMessage()
 * {
 *    SendMessage(WM_CLOSE, 0, 0);
 *    return true;
 * }
 * These overloaded SendMessage and SendDlgItemMessage functions can now fail with LRESULT -1 and
 * ERROR_CAN_NOT_COMPLETE, this is not a standard error for these functions.
 *
 * Note:
 * Some items like the DropDownList and ListView need further initialization during WM_INITDIALOG
 * as some of their settings are not exposed as style flags, but need setting using
 * SendDlgItemMessage().
 * These can be set during init by using the param-parameter to the SpawnDialogBox function.
 *
 * DlgProcCallback:
 * This is a callback definition that is more flexible than the original DLGPROC.
 * The declaration is almost the same as a normal DLGPROC, the only difference is that the CALLBACK
 * keyword must be omitted.
 * Example:
 *     INT_PTR MyCallback(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {}
 * The flexibility comes from this variant being possible to be bound with std::bind.
 */

/*
 * Undefine some Windows defines to make sure calling these functions sends the call to the DlgBase
 * implementation.
 */
#undef SendMessage
#undef SendDlgItemMessage

class DlgBase
{
protected:
    using DlgProcCallback = std::function<INT_PTR(HWND,UINT,WPARAM,LPARAM)>;

    enum class DlgType
    {
        NORMAL,
        TAB_PAGE,
    };

    enum class DlgMode
    {
        PROMPT,
        INDIRECT,
    };

    DlgBase(const std::wstring& caption, SHORT x, SHORT y, SHORT w, SHORT h, DlgType type);
    ~DlgBase();

    /*
     * TODO: Categorize.
     * -- Warepire
     */

    void AddPushButton(const std::wstring& caption,
                       DWORD id,
                       SHORT x, SHORT y,
                       SHORT w, SHORT h,
                       bool default_choice);
    void AddCheckbox(const std::wstring& caption,
                     DWORD id,
                     SHORT x, SHORT y,
                     SHORT w, SHORT h,
                     bool right_hand);
    void AddRadioButton(const std::wstring& caption,
                        DWORD id,
                        SHORT x, SHORT y,
                        SHORT w, SHORT h,
                        bool right_hand,
                        bool group_with_prev);
    void AddUpDownControl(DWORD id, SHORT x, SHORT y, SHORT w, SHORT h);
    void AddEditControl(DWORD id, SHORT x, SHORT y, SHORT w, SHORT h, bool multi_line);
    void AddStaticText(const std::wstring& caption, DWORD id, SHORT x, SHORT y, SHORT w, SHORT h);
    void AddStaticPanel(DWORD id, SHORT x, SHORT y, SHORT w, SHORT h);
    void AddGroupBox(const std::wstring& caption, DWORD id, SHORT x, SHORT y, SHORT w, SHORT h);
    void AddDropDownList(DWORD id, SHORT x, SHORT y, SHORT w, SHORT drop_distance);
    void AddListView(DWORD id,
                     SHORT x, SHORT y,
                     SHORT w, SHORT h,
                     bool single_selection, bool owner_data);
    void AddIPEditControl(DWORD id, SHORT x, SHORT y);
    void AddTabControl(DWORD id, SHORT x, SHORT y, SHORT w, SHORT h);


    INT_PTR SpawnDialogBox(const DlgBase* parent,
                           DlgProcCallback callback,
                           LPARAM init_param,
                           const DlgMode mode);

    LRESULT SendMessage(UINT msg, WPARAM w_param, LPARAM l_param);
    LRESULT SendDlgItemMessage(int item_id, UINT msg, WPARAM w_param, LPARAM l_param);
    BOOL ShowDialogBox(int show_window_cmd);
    BOOL UpdateWindow();
    bool AddTabPageToTabControl(HWND tab_control, unsigned int pos, LPTCITEMW data);

private:
    void AddObject(DWORD ex_style, DWORD style,
                   const std::wstring& window_class,
                   const std::wstring& caption,
                   DWORD id,
                   SHORT x, SHORT y,
                   SHORT w, SHORT h);

    /*
     * Due to DLGPROC types being a C-style function pointer, we cannot std::bind it.
     * Instead we have these generic callbacks, which only purpose is to call the std::bind bound
     * callbacks given to SpawnDialogBox().
     */
    static INT_PTR CALLBACK BaseCallback(HWND window, UINT msg, WPARAM w_param, LPARAM l_param);
    INT_PTR LocalCallback(HWND window, UINT msg, WPARAM w_param, LPARAM l_param);

    struct LParamData
    {
        LParamData(LPARAM init_data, DlgBase* dialog) :
            m_original_data(init_data),
            m_dialog(dialog) {}

        LPARAM m_original_data;
        DlgBase* m_dialog;
    };

    static std::map<HWND, DlgBase*> ms_hwnd_to_dlgbase_map;
    std::vector<BYTE> m_window;
    HWND m_handle;
    DlgProcCallback m_callback;
};

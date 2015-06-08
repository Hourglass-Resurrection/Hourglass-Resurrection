/*
 * (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#include <Windows.h>
#include <CommCtrl.h>

#include <functional>
#include <string>
#include <vector>

/*
 * ------------------------------------------------------------------------------------------------
 *                                            USAGE
 * ------------------------------------------------------------------------------------------------
 *
 * Usage is simple. Just remember that everything is in DialogPoints and not in pixels.
 *
 * This construction allows for enum IDs for the object IDs, and it is encouraged to use this
 * method to declare them in order to avoid ID collisions. It is also encouraged to keep the IDs
 * private to the window class.
 *
 * When creating an Indirect dialog, Spawn() will return -1 on creation failure and 0 on success.
 * When creating a Prompt dialog, Spawn() will return the result of the dialog box when it exits.
 *
 * The purpose of SendMessage and SendDlgItemMessage is to send messages to the window and it's
 * controls, without keeping track of the HWND.
 * Don't just proxy this call in your window class, wrap it to obscure the message IDs as well.
 * You're meant to have class members that looks like this (example code, do not actually use):
 * bool SendCloseMessage()
 * {
 *    base.SendMessage(WM_CLOSE, 0, 0);
 *    return true;
 * }
 * These overloaded SendMessage and SendDlgItemMessage functions can now fail with LRESULT -1 and
 * ERROR_CAN_NOT_COMPLETE, this is not a standard error for these functions.
 *
 * Note:
 * Some items like the DropDownList and ListView need further initialization during WM_INITDIALOG
 * as some of their settings are not exposed as style flags, but need setting using SendMessage.
 * These can be set during init by using the param-parameter to the SpawnDialogBox function.
 *
 * Important:
 * If ListView is made editable LVN_ENDLABELEDIT must be caught in the message handler and
 * return TRUE. Otherwise the data is never saved.
 *
 * Hint:
 
 */

/*
 * DlgProcCallback type, a callback that contains a more flexible DLGPROC-like function.
 * The declaration is almost the same as a normal DLGPROC, the only difference is that CALLBACK
 * needs to be omitted.
 * Example:
 *     INT_PTR MyCallback(HWND window,  UINT msg, WPARAM wparam, LPARAM lparam) {}
 * The flexibility comes from this variant being possible to be bound, meaning std::bind
 * can be used to create a callback of a non-static class member function.
 */
typedef std::function<INT_PTR(HWND,UINT,WPARAM,LPARAM)> DlgProcCallback;

/*
 * A lot of object types are missing, please add them as necessary.
 * Refer to the currently implemented objects when creating new ones.
 * -- Warepire
 */

class DlgBase
{
public:
    enum DlgMode
    {
        PROMPT,
        INDIRECT,
    };

    DlgBase(std::wstring caption, SHORT x, SHORT y, SHORT w, SHORT h);
    ~DlgBase();

    /*
     * TODO: Categorize.
     * -- Warepire
     */

    void AddPushButton(std::wstring caption,
                       DWORD id,
                       SHORT x, SHORT y,
                       SHORT w, SHORT h,
                       bool default);
    void AddCheckbox(std::wstring caption,
                     DWORD id,
                     SHORT x, SHORT y,
                     SHORT w, SHORT h,
                     bool right_hand);
    void AddRadioButton(std::wstring caption,
                        DWORD id,
                        SHORT x, SHORT y,
                        SHORT w, SHORT h,
                        bool right_hand,
                        bool group_with_prev);
    void AddUpDownControl(DWORD id, SHORT x, SHORT y, SHORT w, SHORT h);
    void AddEditControl(DWORD id, SHORT x, SHORT y, SHORT w, SHORT h, bool multi_line);
    void AddStaticText(std::wstring caption, DWORD id, SHORT x, SHORT y, SHORT w, SHORT h);
    void AddStaticPanel(DWORD id, SHORT x, SHORT y, SHORT w, SHORT h);
    void AddGroupBox(std::wstring caption, DWORD id, SHORT x, SHORT y, SHORT w, SHORT h);
    void AddDropDownList(DWORD id, SHORT x, SHORT y, SHORT w, SHORT drop_distance);
    void AddListView(DWORD id,
                     SHORT x, SHORT y,
                     SHORT w, SHORT h,
                     bool editable, bool single_selection, bool owner_data);


    INT_PTR SpawnDialogBox(HINSTANCE instance,
                           HWND parent,
                           DlgProcCallback callback,
                           LPARAM init_param,
                           DlgMode mode);

    LRESULT SendMessage(UINT msg, WPARAM w_param, LPARAM l_param);
    LRESULT SendDlgItemMessage(int item_id, UINT msg, WPARAM w_param, LPARAM l_param);

private:
    void AddObject(DWORD ex_style, DWORD style,
                   std::wstring window_class,
                   std::wstring& caption,
                   DWORD id,
                   SHORT x, SHORT y,
                   SHORT w, SHORT h);

    /*
     * Due to DLGPROC types being a C-style function pointer, we cannot std::bind it.
     * Instead we have this generic callback, which only purpose is to call std::bind bound
     * callbacks.
     */
    static std::map<HWND, DlgProcCallback> callback_map;
    static INT_PTR CALLBACK BaseCallback(HWND window, UINT msg, WPARAM w_param, LPARAM l_param);

    struct LParamData
    {
        LPARAM original_data;
        DlgProcCallback callback;
    };

    std::vector<BYTE> window;
    HWND handle;
};
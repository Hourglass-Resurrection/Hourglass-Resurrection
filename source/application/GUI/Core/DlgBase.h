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

#include "MenuBase.h"

/*
 * Undefine some Windows defines to make sure calling these functions sends the call to the DlgBase
 * implementation.
 */
#undef SendMessage
#undef SendDlgItemMessage

class CallbackBase;

class DlgBase
{
protected:
    enum class DlgMode
    {
        PROMPT,
        INDIRECT,
    };

    DlgBase(const std::wstring& caption, SHORT x, SHORT y, SHORT w, SHORT h);
    ~DlgBase();

    INT_PTR SpawnDialogBox(const DlgBase* parent, const DlgMode mode);

    LRESULT SendMessage(UINT msg, WPARAM wparam, LPARAM lparam);
    BOOL ShowDialogBox(int show_window_cmd);
    BOOL UpdateWindow();
    void SetReturnCode(INT_PTR return_code);
    BOOL DestroyDialog();

    void RegisterCloseEventCallback(std::function<bool()> cb);

private:
    LRESULT SendDlgItemMessage(int item_id, UINT msg, WPARAM wparam, LPARAM lparam);
    bool SetFocus(DWORD id);

    DWORD GetNextID();
    SIZE_T AddObject(const std::vector<BYTE>& object);
    void SetNewStyle(SIZE_T obj_offset, DWORD ex_style, DWORD style);

    void RegisterWmControlCallback(DWORD id, std::function<bool(WORD)> cb);

    bool DestroyCallback();
    bool NcDestroyCallback();

    static DWORD ms_ref_count;

    std::vector<BYTE> m_window;
    HWND m_handle;
    DlgMode m_mode;
    bool m_return_code_set;
    INT_PTR m_return_code;

    bool SetMenuBar();
    void AppendMenuItems(const MenuData* item);
    std::vector<BYTE> m_menu;
    MenuData m_menu_data;
    HMENU m_menu_handle;

    DWORD m_next_id;

    std::map<UINT, std::vector<std::unique_ptr<CallbackBase>>> m_wm_command_callbacks;
    std::map<UINT, std::vector<std::unique_ptr<CallbackBase>>> m_message_callbacks;

    INT_PTR DlgCallback(HWND window, UINT msg, WPARAM wparam, LPARAM lparam);
    /*
     * Due to DLGPROC types being a C-style function pointer, we cannot std::bind it.
     * Instead we have to use this generic callback, which only purpose is to call registered callbacks.
     */
    static INT_PTR CALLBACK BaseCallback(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
    static std::map<HWND, DlgBase*> ms_hwnd_dlgbase_map;

    friend class MenuItemBase;

    template<typename T, std::size_t>
    friend class ObjBase;
};

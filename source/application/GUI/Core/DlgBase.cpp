/*
 * (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include <Windows.h>
#include <CommCtrl.h>

#include <cassert>
#include <cwchar>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "shared/Alignment.h"

#include "../Objects/StaticText.h"

#include "DlgBase.h"

/*
 * These classes provide type erasure on callback functions to simplify
 * the call process from the message loop to the event handlers.
 */
class CallbackBase
{
public:
    virtual ~CallbackBase()
    {
    }
    virtual bool Callback(LPARAM lparam, WPARAM wparam) = 0;
};

class Callback0 : public CallbackBase
{
public:
    Callback0(std::function<bool()> cb) : m_callback(cb)
    {
    }
    virtual bool Callback(LPARAM lparam, WPARAM wparam)
    {
        return m_callback();
    }

private:
    std::function<bool()> m_callback;
};

template<typename T>
class CallbackLparam : public CallbackBase
{
public:
    CallbackLparam(std::function<bool(T)> cb) : m_callback(cb)
    {
    }
    virtual bool Callback(LPARAM lparam, WPARAM wparam)
    {
        return m_callback(reinterpret_cast<T>(lparam));
    }

private:
    std::function<bool(T)> m_callback;
};

template<typename T>
class CallbackWparam : public CallbackBase
{
public:
    CallbackWparam(std::function<bool(T)> cb) : m_callback(cb)
    {
    }
    virtual bool Callback(LPARAM lparam, WPARAM wparam)
    {
        return m_callback(reinterpret_cast<T>(wparam));
    }

private:
    std::function<bool(T)> m_callback;
};

template<typename T, typename U>
class CallbackBoth : public CallbackBase
{
public:
    CallbackBoth(std::function<bool(T, U)> cb) : m_callback(cb)
    {
    }
    virtual bool Callback(LPARAM lparam, WPARAM wparam)
    {
        return m_callback(reinterpret_cast<T>(lparam), reinterpret_cast<U>(wparam));
    }

private:
    std::function<bool(T, U)> m_callback;
};

class CallbackWmCommand : public CallbackBase
{
public:
    CallbackWmCommand(std::function<bool(WORD)> cb) : m_callback(cb)
    {
    }
    virtual bool Callback(LPARAM lparam, WPARAM wparam)
    {
        return m_callback(HIWORD(wparam));
    }

private:
    std::function<bool(WORD)> m_callback;
};

namespace
{
    constexpr WCHAR TYPE_FACE[] = L"MS Shell Dlg";

#pragma pack(push, 1)
    /*
     * On MSDN these are declared as 1 struct, but, since the title-member is of variadic
     * length it's declared as 2 structs here.
     */
    struct DLGTEMPLATEEX_1
    {
        WORD dlgVer;
        WORD signature;
        DWORD helpID;
        DWORD exStyle;
        DWORD style;
        WORD cDlgItems;
        short x;
        short y;
        short cx;
        short cy;
        WCHAR menu;
        WCHAR windowClass;
        WCHAR title;
    };
    struct DLGTEMPLATEEX_2
    {
        WORD pointsize;
        WORD weight;
        BYTE italic;
        BYTE charset;
        WCHAR typeface[ARRAYSIZE(TYPE_FACE)];
    };
    /*
     * Hacky... But avoids weird magic
     * -- Warepire
     */
    struct DLGITEMTEMPLATEEX_MINIMAL
    {
        DWORD reserved;
        DWORD exStyle;
        DWORD style;
    };

    struct MENUEX_TEMPLATE_HEADER
    {
        WORD wVersion;
        WORD wOffset;
        DWORD dwHelpId;
    };
    /*
     * Hacky... But avoids weird magic
     * -- Warepire
     */
    struct MENUEX_TEMPLATE_ITEM_MINIMAL
    {
        DWORD reserved[3];
        WORD bResInfo;
    };
#pragma pack(pop)
}

std::map<HWND, DlgBase*> DlgBase::ms_hwnd_dlgbase_map;

DWORD DlgBase::ms_ref_count = 0;

namespace
{
    HINSTANCE gs_instance;
}

/*
 * wmemcpy is used over wcscpy, swprintf etc in this code. This is due to these methods end up
 * corrupting the DLGTEMPLATE structures causing Windows to read memory continously until it
 * reaches a memory access violation exception (and thus the program crashes).
 * -- Warepire
 */
DlgBase::DlgBase(const std::wstring& caption, SHORT x, SHORT y, SHORT w, SHORT h)
    : m_handle(nullptr)
    , m_mode(DlgMode::INDIRECT)
    , m_return_code_set(false)
    , m_return_code(0)
    , m_window(AlignValueTo<sizeof(DWORD)>(sizeof(DLGTEMPLATEEX_1) + sizeof(DLGTEMPLATEEX_2)
                                           + (caption.size() * sizeof(WCHAR))))
    , m_next_id(0)
{
    /*
     * A bit hacky to avoid a special init function just to pass the isntance to us.
     * -- Warepire
     */
    if (ms_ref_count == 0)
    {
        gs_instance = GetModuleHandleW(nullptr);
    }

    ms_ref_count++;

    m_message_callbacks[WM_DESTROY].emplace_back(
        std::make_unique<Callback0>(std::bind(&DlgBase::DestroyCallback, this)));
    m_message_callbacks[WM_NCDESTROY].emplace_back(
        std::make_unique<Callback0>(std::bind(&DlgBase::NcDestroyCallback, this)));
    m_message_callbacks[WM_INITDIALOG].emplace_back(
        std::make_unique<Callback0>(std::bind(&DlgBase::SetMenuBar, this)));

    auto window_1 = reinterpret_cast<DLGTEMPLATEEX_1*>(m_window.data());
    window_1->dlgVer = 0x0001;
    window_1->signature = 0xFFFF;
    window_1->style = DS_SETFONT | DS_FIXEDSYS | DS_MODALFRAME | WS_POPUP | WS_CAPTION | WS_SYSMENU
                      | WS_MINIMIZEBOX;

    window_1->x = x;
    window_1->y = y;
    window_1->cx = w;
    window_1->cy = h;

    if (!caption.empty())
    {
        wmemcpy(&window_1->title, caption.data(), caption.size());
    }

    auto window_2 = reinterpret_cast<DLGTEMPLATEEX_2*>(m_window.data() + sizeof(DLGTEMPLATEEX_1)
                                                       + (caption.size() * sizeof(WCHAR)));

    window_2->pointsize = 0x0008;
    window_2->weight = FW_NORMAL;
    window_2->italic = FALSE;
    window_2->charset = DEFAULT_CHARSET;
    wmemcpy(window_2->typeface, TYPE_FACE, ARRAYSIZE(TYPE_FACE) - 1);

    /*
     * Hack to resize windows when adding menus.
     * -- Warepire
     */
    StaticText(L"", 0, 0, 1, 1, this);
}

DlgBase::~DlgBase()
{
    ms_ref_count--;
}

DWORD DlgBase::GetNextID()
{
    return m_next_id++;
}

SIZE_T DlgBase::AddObject(const std::vector<BYTE>& object)
{
    SIZE_T old_size = m_window.size();
    SIZE_T new_size = AlignValueTo<sizeof(DWORD)>(old_size + object.size());

    m_window.reserve(new_size);
    m_window.insert(m_window.end(), object.begin(), object.end());
    while (new_size > m_window.size())
    {
        m_window.emplace_back(0);
    }

    auto window = reinterpret_cast<DLGTEMPLATEEX_1*>(m_window.data());
    window->cDlgItems++;

    return old_size;
}

void DlgBase::SetNewStyle(SIZE_T obj_offset, DWORD ex_style, DWORD style)
{
    auto obj = reinterpret_cast<DLGITEMTEMPLATEEX_MINIMAL*>(m_window.data() + obj_offset);
    obj->exStyle = ex_style;
    obj->style = style;
}

INT_PTR DlgBase::SpawnDialogBox(const DlgBase* parent, const DlgMode mode)
{
    INT_PTR result;
    HWND parent_hwnd = (parent != nullptr ? parent->m_handle : nullptr);

    if (m_handle == nullptr)
    {
        m_mode = mode;
        switch (mode)
        {
        case DlgMode::PROMPT:
            result = DialogBoxIndirectParamW(gs_instance,
                                             reinterpret_cast<LPCDLGTEMPLATEW>(m_window.data()),
                                             parent_hwnd,
                                             BaseCallback,
                                             reinterpret_cast<LPARAM>(this));
            return result;
        case DlgMode::INDIRECT:
            /*
             * Don't save the HWND directly here, save it from the callback to unify the saving
             * process with the PROMPT dialogs. CreateDialog[X] will not return until the window
             * has processed the WM_INITDIALOG message.
             */
            HWND ret =
                CreateDialogIndirectParamW(gs_instance,
                                           reinterpret_cast<LPCDLGTEMPLATEW>(m_window.data()),
                                           parent_hwnd,
                                           BaseCallback,
                                           reinterpret_cast<LPARAM>(this));
            if (ret == nullptr)
            {
                return -1;
            }
            return 0;
        }
    }

    throw std::exception("DlgBase::SpawnDialogBox() called on spawned dialog box");
}

LRESULT DlgBase::SendMessage(UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (m_handle == nullptr)
    {
        auto cb = [this, msg, wparam, lparam]() {
            ::SendMessageW(m_handle, msg, wparam, lparam);
            return true;
        };
        m_message_callbacks[WM_INITDIALOG].emplace_back(std::make_unique<Callback0>(cb));
        return 0;
    }
    return ::SendMessageW(m_handle, msg, wparam, lparam);
}

LRESULT DlgBase::SendDlgItemMessage(int item_id, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (m_handle == nullptr)
    {
        auto cb = [this, item_id, msg, wparam, lparam]() {
            ::SendDlgItemMessageW(m_handle, item_id, msg, wparam, lparam);
            return true;
        };
        m_message_callbacks[WM_INITDIALOG].emplace_back(std::make_unique<Callback0>(cb));
        return 0;
    }
    else
    {
        return ::SendDlgItemMessageW(m_handle, item_id, msg, wparam, lparam);
    }
}

bool DlgBase::SetFocus(DWORD id)
{
    if (m_handle == nullptr)
    {
        auto cb = [this, id]() {
            ::SetFocus(GetDlgItem(m_handle, id));
            return true;
        };
        m_message_callbacks[WM_INITDIALOG].emplace_back(std::make_unique<Callback0>(cb));
        return true;
    }
    else
    {
        HWND rv = ::SetFocus(GetDlgItem(m_handle, id));
        return (rv != nullptr);
    }
}

BOOL DlgBase::ShowDialogBox(int show_window_cmd)
{
    if (m_handle == nullptr)
    {
        throw std::exception("DlgBase::ShowDialogBox() called before dialog was created.");
    }

    return ::ShowWindow(m_handle, show_window_cmd);
}

BOOL DlgBase::UpdateWindow()
{
    if (m_handle == nullptr)
    {
        throw std::exception("DlgBase::UpdateWindow() called before dialog was created.");
    }

    return ::UpdateWindow(m_handle);
}

void DlgBase::SetReturnCode(INT_PTR return_code)
{
    m_return_code_set = true;
    m_return_code = return_code;
}

BOOL DlgBase::DestroyDialog()
{
    if (m_handle == nullptr)
    {
        throw std::exception("DlgBase::DestroyDialog() called before dialog was created.");
    }

    if (m_mode == DlgMode::INDIRECT)
    {
        return ::DestroyWindow(m_handle);
    }
    else if (m_mode == DlgMode::PROMPT)
    {
        if (!m_return_code_set)
        {
            throw std::exception(
                "DlgBase::DestroyDialog() called on a Prompt without setting return code.");
        }
        return ::EndDialog(m_handle, m_return_code);
    }
    return FALSE;
}

bool DlgBase::SetMenuBar()
{
    if (m_menu_data.m_children.empty())
    {
        return true;
    }

    m_menu.resize(sizeof(MENUEX_TEMPLATE_HEADER));
    auto header = reinterpret_cast<MENUEX_TEMPLATE_HEADER*>(m_menu.data());
    header->wVersion = 0x01;
    header->wOffset = 0x04;

    if (m_menu_handle != nullptr)
    {
        DestroyMenu(m_menu_handle);
        m_menu_handle = nullptr;
    }

    AppendMenuItems(&m_menu_data);

    m_menu_handle = LoadMenuIndirectW(reinterpret_cast<LPMENUTEMPLATEW>(m_menu.data()));

    if (m_menu_handle == nullptr)
    {
        return false;
    }

    /*
     * Super-hacky: Since this little label is added first, it has ID 0.
     * -- Warepire
     */
    HWND anchor_hwnd = GetDlgItem(m_handle, 0);
    RECT old_rect = {};
    RECT new_rect = {};
    GetWindowRect(anchor_hwnd, &old_rect);

    BOOL rv = ::SetMenu(m_handle, m_menu_handle);

    /*
     * If setting the menu pushed the window contents down,
     * extend the bottom of the window by the same amount.
     */
    GetWindowRect(anchor_hwnd, &new_rect);
    int change = new_rect.top - old_rect.top;
    if (change != 0)
    {
        RECT rect;
        GetWindowRect(m_handle, &rect);
        SetWindowPos(m_handle,
                     nullptr,
                     0,
                     0,
                     rect.right - rect.left,
                     change + rect.bottom - rect.top,
                     SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
    }

    return (rv == TRUE);
}

void DlgBase::RegisterCloseEventCallback(std::function<bool()> cb)
{
    m_message_callbacks[WM_CLOSE].emplace_back(std::make_unique<Callback0>(cb));
}

void DlgBase::RegisterWmControlCallback(DWORD id, std::function<bool(WORD)> cb)
{
    m_wm_command_callbacks[id].emplace_back(std::make_unique<CallbackWmCommand>(cb));
}

bool DlgBase::DestroyCallback()
{
    /*
     * Post Quit message regardless of m_return_code_set, assume everything is ok (return 0)
     */
    if (ms_ref_count == 1)
    {
        PostQuitMessage(static_cast<int>(m_return_code));
    }
    return true;
}

bool DlgBase::NcDestroyCallback()
{
    m_handle = nullptr;
    m_message_callbacks.clear();
    m_wm_command_callbacks.clear();
    return true;
}

void DlgBase::AppendMenuItems(const MenuData* item)
{
    auto it = item->m_children.begin();
    while (it != item->m_children.end())
    {
        std::size_t old_size = m_menu.size();
        std::size_t new_size = AlignValueTo<sizeof(DWORD)>(old_size + it->get()->m_menu.size());
        m_menu.reserve(new_size);
        m_menu.insert(m_menu.end(), it->get()->m_menu.begin(), it->get()->m_menu.end());
        while (new_size > m_menu.size())
        {
            m_menu.emplace_back(0);
        }
        AppendMenuItems(it->get());
        if ((++it) == item->m_children.end())
        {
            auto last = reinterpret_cast<MENUEX_TEMPLATE_ITEM_MINIMAL*>(m_menu.data() + old_size);
            last->bResInfo |= 0x80;
        }
    }
}

INT_PTR DlgBase::DlgCallback(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    /*
     * TODO: No support for WM_NOTIFY
     */
    bool rv = false;
    auto& cb_map = message == WM_COMMAND ? m_wm_command_callbacks : m_message_callbacks;
    auto& cb = cb_map.find(message == WM_COMMAND ? LOWORD(wparam) : message);
    if (cb != cb_map.end())
    {
        for (auto& c : cb->second)
        {
            rv |= c->Callback(wparam, lparam);
            if (message == WM_INITDIALOG)
            {
                /*
                 * Special case, returning FALSE from WM_INITDIALOG tells Windows to not
                 * change the focus of controls, and we can thus set it ourselves.
                 */
                rv = false;
            }
        }
    }

    return rv ? TRUE : FALSE;
}

INT_PTR CALLBACK DlgBase::BaseCallback(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (message == WM_INITDIALOG)
    {
        /*
         * Window init special case: Map the DlgBase pointer to the window handle,
         * And set the m_handle member of DlgBase. Ugly hack that can't be avoided.
         */
        ms_hwnd_dlgbase_map[window] = reinterpret_cast<DlgBase*>(lparam);
        ms_hwnd_dlgbase_map[window]->m_handle = window;
    }

    auto dlg_from_hwnd = ms_hwnd_dlgbase_map.find(window);
    if (dlg_from_hwnd == ms_hwnd_dlgbase_map.end())
    {
        return FALSE;
    }
    INT_PTR rv = dlg_from_hwnd->second->DlgCallback(window, message, wparam, lparam);

    if (message == WM_NCDESTROY)
    {
        ms_hwnd_dlgbase_map.erase(window);
    }

    return rv;
}

/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

#include "DlgBase.h"

template<typename T, std::size_t class_len>
class ObjBase
{
public:
    /*
     * Generic settings that can be set on any object type
     */
    T& SetEnabled(bool enabled)
    {
        SetUnsetStyleBits(0, WS_DISABLED, enabled ? SetBits::Unset : SetBits::Set);
        return *static_cast<T*>(this);
    }

    T& SetFocus()
    {
        auto obj = reinterpret_cast<DLGITEMTEMPLATEEX*>(m_object.data());
        m_dlg->SetFocus(obj->id);
        return *static_cast<T*>(this);
    }

protected:
    enum class SetBits
    {
        Set,
        Unset,
    };

    ObjBase(const std::wstring& title,
            const WCHAR* const window_class,
            DWORD ex_style,
            DWORD style,
            short x,
            short y,
            short w,
            short h,
            DlgBase* dlg)
        /*
         * Struct size + title-len + extraCount
         */
        : m_object(sizeof(DLGITEMTEMPLATEEX) + (title.size() * sizeof(WCHAR)) + sizeof(WORD))
        , m_dlg(dlg)
    {
        auto obj = reinterpret_cast<DLGITEMTEMPLATEEX*>(m_object.data());
        obj->exStyle = ex_style;
        obj->style = WS_CHILD | WS_VISIBLE | style;
        obj->x = x;
        obj->y = y;
        obj->cx = w;
        obj->cy = h;
        obj->id = m_dlg->GetNextID();
        wmemcpy(obj->windowClass, window_class, class_len);
        if (!title.empty())
        {
            wmemcpy(&obj->title, title.data(), title.size());
        }
        m_obj_id = m_dlg->AddObject(m_object);
    }

    void SetUnsetStyleBits(DWORD ex_style, DWORD style, SetBits set)
    {
        auto obj = reinterpret_cast<DLGITEMTEMPLATEEX*>(m_object.data());
        if (set == SetBits::Set)
        {
            obj->exStyle |= ex_style;
            obj->style |= style;
        }
        else
        {
            obj->exStyle &= ~(ex_style);
            obj->style &= ~(style);
        }
        m_dlg->SetNewStyle(m_obj_id, obj->exStyle, obj->style);
    }

    void SendDlgMessage(UINT message, WPARAM wparam, LPARAM lparam)
    {
        auto obj = reinterpret_cast<DLGITEMTEMPLATEEX*>(m_object.data());
        m_dlg->SendDlgItemMessage(obj->id, message, wparam, lparam);
    }

    void RegisterWmCommandHandler(std::function<bool(WORD)>& cb)
    {
        auto obj = reinterpret_cast<DLGITEMTEMPLATEEX*>(m_object.data());
        m_dlg->RegisterWmControlCallback(obj->id, cb);
    }

private:
#pragma pack(push, 1)
    struct DLGITEMTEMPLATEEX
    {
        DWORD helpID;
        DWORD exStyle;
        DWORD style;
        short x;
        short y;
        short cx;
        short cy;
        DWORD id;
        WCHAR windowClass[class_len];
        WCHAR title;
        /*
         * WORD extraCount; <- This is always 0
         */
    };
#pragma pack(pop)
    std::vector<BYTE> m_object;
    /*
     * This object ID is not the same as the Windows ID
     */
    SIZE_T m_obj_id;
    DlgBase* m_dlg;
};

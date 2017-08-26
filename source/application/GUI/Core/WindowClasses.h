/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

static constexpr WCHAR BUTTON_WND_CLASS[2] = { L'\xFFFF', L'\x0080' };
static constexpr WCHAR EDIT_WND_CLASS[2] = { L'\xFFFF', L'\x0081' };
static constexpr WCHAR STATIC_WND_CLASS[2] = { L'\xFFFF', L'\x0082' };
static constexpr WCHAR LISTBOX_WND_CLASS[2] = { L'\xFFFF', L'\x0083' };
static constexpr WCHAR COMBOBOX_WND_CLASS[2] = { L'\xFFFF', L'\x0085' };

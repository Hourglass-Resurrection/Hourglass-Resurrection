/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cstddef>
#include <memory>
#include <vector>

struct MenuData
{
    std::vector<BYTE> m_menu;
    std::vector<std::unique_ptr<MenuData>> m_children;
};

class MenuBase
{
protected:
    MenuData* m_data;

    friend class DlgBase;
    friend class MenuItemBase;
};

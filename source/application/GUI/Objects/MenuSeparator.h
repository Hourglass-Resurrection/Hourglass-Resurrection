/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#include "../Core/MenuItemBase.h"

class DlgBase;

class MenuSeparator : public MenuItemBase
{
public:
    MenuSeparator(MenuItemBase* parent, DlgBase* dlg);
};

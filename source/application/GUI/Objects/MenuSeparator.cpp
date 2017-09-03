/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#include "../Core/MenuItemBase.h"

#include "MenuSeparator.h"

class DlgBase;

MenuSeparator::MenuSeparator(MenuItemBase* parent, DlgBase* dlg)
    : MenuItemBase(L"", L"", false, parent, dlg)
{
}

/*
 * Copyright (c) 2015- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#include <windows.h>

class WaitableBool
{
public:
	WaitableBool(bool initialState = false);
	~WaitableBool();
	void WaitUntilTrue() const;
	void WaitUntilFalse() const;
	void operator =(bool set);
	operator bool() const;

private:
	HANDLE m_event_true;
	HANDLE m_event_false;
	bool m_value;
};

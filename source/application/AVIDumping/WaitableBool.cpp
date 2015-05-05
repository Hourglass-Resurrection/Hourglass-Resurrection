/*
* (c) 2015- Hourglass Resurrection Team
* Hourglass Resurrection is licensed under GPL v2.
* Refer to the file COPYING.txt in the project root.
*/

#include "WaitableBool.h"

WaitableBool::WaitableBool(bool initialState)
	: m_value(initialState)
{
	m_event_true = CreateEvent(nullptr, TRUE, initialState, nullptr);
	m_event_false = CreateEvent(nullptr, TRUE, !initialState, nullptr);
}

WaitableBool::~WaitableBool()
{
	CloseHandle(m_event_true);
	CloseHandle(m_event_false);
}

void WaitableBool::WaitUntilTrue() const
{
	if (!m_value)
	{
		WaitForSingleObject(m_event_true, INFINITE);
	}
}

void WaitableBool::WaitUntilFalse() const
{
	if (m_value)
	{
		WaitForSingleObject(m_event_false, INFINITE);
	}
}

void WaitableBool::operator =(bool set)
{
	m_value = set;

	if (set)
	{
		SetEvent(m_event_true);
		ResetEvent(m_event_false);
	}
	else
	{
		ResetEvent(m_event_true);
		SetEvent(m_event_false);
	}
}

WaitableBool::operator bool() const
{
	return m_value;
}

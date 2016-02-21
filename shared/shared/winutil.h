/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

struct AutoCritSect
{
	AutoCritSect(CRITICAL_SECTION* cs) : m_cs(cs) { EnterCriticalSection(m_cs); }
	~AutoCritSect() { LeaveCriticalSection(m_cs); }
	CRITICAL_SECTION* m_cs;
};

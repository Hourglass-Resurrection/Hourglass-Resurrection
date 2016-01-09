/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#include <windows.h>

UINT LocaleToCodePage(LCID locale);
DWORD LocaleToCharset(LCID locale);

// it's not the most robust implementation but I'm just going for a little code reuse here.
// leaves result on the stack in a variable of a name specified by wstr.
#define str_to_wstr(wstr, str, codepage) \
	int len##wstr = MultiByteToWideChar(codepage, 0, str, -1, NULL, 0); \
	WCHAR* wstr = static_cast<WCHAR*>(MemoryManager::Allocate(len##wstr * sizeof(WCHAR), MemoryManager::ALLOC_WRITE | MemoryManager::ALLOC_INTERNAL)); \
	memset(wstr, 0, len##wstr*sizeof(WCHAR)); \
	MultiByteToWideChar(codepage, 0, str, -1, wstr, len##wstr);

/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef LOCALE_H_INCL
#define LOCALE_H_INCL

#include <windows.h>

inline UINT LocaleToCodePage(LCID locale)
{
    switch (locale)
    {
    case 1041:
        return 932; // shift-JIS
    case 2052:
        return 936; // simplified chinese
    case 1042:
        return 949; // korean
    default: // most others NYI
    case 1033:
        return 1252;
    }
}
inline DWORD LocaleToCharset(LCID locale)
{
    switch (locale)
    {
    case 1041:
        return SHIFTJIS_CHARSET;
    case 2052:
        return GB2312_CHARSET;
    case 1042:
        return HANGUL_CHARSET;
    default: // most others NYI
    case 1033:
        return ANSI_CHARSET;
    }
}

// it's not the most robust implementation but I'm just going for a little code reuse here.
// leaves result on the stack in a variable of a name specified by wstr.
#define str_to_wstr(wstr, str, codepage)                                \
    int len##wstr = MultiByteToWideChar(codepage, 0, str, -1, NULL, 0); \
    WCHAR* wstr = (WCHAR*) _alloca(len##wstr * sizeof(WCHAR));          \
    memset(wstr, 0, len##wstr * sizeof(WCHAR));                         \
    MultiByteToWideChar(codepage, 0, str, -1, wstr, len##wstr);

#endif // LOCALE_H_INCL

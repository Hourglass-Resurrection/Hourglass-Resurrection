/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#include "../intercept.h"

namespace Hooks
{
    HOOK_FUNCTION_DECLARE(LONG, APIENTRY, RegOpenKeyA, HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult);
    HOOK_FUNCTION_DECLARE(LONG,
                          APIENTRY,
                          RegOpenKeyExA,
                          HKEY hKey,
                          LPCSTR lpSubKey,
                          DWORD ulOptions,
                          REGSAM samDesired,
                          PHKEY phkResult);
    HOOK_FUNCTION_DECLARE(LONG,
                          APIENTRY,
                          RegEnumKeyA,
                          HKEY hKey,
                          DWORD dwIndex,
                          LPCSTR lpName,
                          DWORD cchName);
    HOOK_FUNCTION_DECLARE(LONG,
                          APIENTRY,
                          RegQueryValueExA,
                          HKEY hKey,
                          LPCTSTR lpValueName,
                          LPDWORD lpReserved,
                          LPDWORD lpType,
                          LPBYTE lpData,
                          LPDWORD lpcbData);
    HOOK_FUNCTION_DECLARE(LONG,
                          APIENTRY,
                          RegOpenKeyW,
                          HKEY hKey,
                          LPCWSTR lpSubKey,
                          PHKEY phkResult);
    HOOK_FUNCTION_DECLARE(LONG,
                          APIENTRY,
                          RegOpenKeyExW,
                          HKEY hKey,
                          LPCWSTR lpSubKey,
                          DWORD ulOptions,
                          REGSAM samDesired,
                          PHKEY phkResult);
    HOOK_FUNCTION_DECLARE(LONG,
                          APIENTRY,
                          RegEnumKeyW,
                          HKEY hKey,
                          DWORD dwIndex,
                          LPCWSTR lpName,
                          DWORD cchName);
    HOOK_FUNCTION_DECLARE(LONG,
                          APIENTRY,
                          RegQueryValueExW,
                          HKEY hKey,
                          LPCWSTR lpValueName,
                          LPDWORD lpReserved,
                          LPDWORD lpType,
                          LPBYTE lpData,
                          LPDWORD lpcbData);
    HOOK_FUNCTION_DECLARE(NTSTATUS,
                          NTAPI,
                          NtQueryKey,
                          IN HANDLE KeyHandle,
                          IN DWORD KeyInformationClass,
                          OUT PVOID KeyInformation,
                          IN ULONG Length,
                          OUT PULONG ResultLength);

    // close enough?
    HOOK_FUNCTION_DECLARE(int, WINAPI, GetSystemMetrics, int nIndex);
    HOOK_FUNCTION_DECLARE(UINT, WINAPI, GetACP);
    HOOK_FUNCTION_DECLARE(UINT, WINAPI, GetOEMCP);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, GetCPInfo, UINT CodePage, LPCPINFO lpCPInfo);
    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          GetCPInfoExA,
                          UINT CodePage,
                          DWORD dwFlags,
                          LPCPINFOEXA lpCPInfoEx);
    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          GetCPInfoExW,
                          UINT CodePage,
                          DWORD dwFlags,
                          LPCPINFOEXW lpCPInfoEx);
    HOOK_FUNCTION_DECLARE(int,
                          WINAPI,
                          GetLocaleInfoA,
                          LCID Locale,
                          LCTYPE LCType,
                          LPSTR lpLCData,
                          int cchData);
    HOOK_FUNCTION_DECLARE(int,
                          WINAPI,
                          GetLocaleInfoW,
                          LCID Locale,
                          LCTYPE LCType,
                          LPWSTR lpLCData,
                          int cchData);
    HOOK_FUNCTION_DECLARE(int,
                          WINAPI,
                          LCMapStringA,
                          LCID Locale,
                          DWORD dwMapFlags,
                          LPCSTR lpSrcStr,
                          int cchSrc,
                          LPSTR lpDestStr,
                          int cchDest);
    HOOK_FUNCTION_DECLARE(int,
                          WINAPI,
                          LCMapStringW,
                          LCID Locale,
                          DWORD dwMapFlags,
                          LPCWSTR lpSrcStr,
                          int cchSrc,
                          LPWSTR lpDestStr,
                          int cchDest);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, IsDBCSLeadByte, BYTE TestChar);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, IsDBCSLeadByteEx, UINT CodePage, BYTE TestChar);
    HOOK_FUNCTION_DECLARE(int,
                          WINAPI,
                          MultiByteToWideChar,
                          UINT CodePage,
                          DWORD dwFlags,
                          LPCSTR lpMultiByteStr,
                          int cbMultiByte,
                          LPWSTR lpWideCharStr,
                          int cchWideChar);
    HOOK_FUNCTION_DECLARE(int,
                          WINAPI,
                          WideCharToMultiByte,
                          UINT CodePage,
                          DWORD dwFlags,
                          LPCWSTR lpWideCharStr,
                          int cchWideChar,
                          LPSTR lpMultiByteStr,
                          int cbMultiByte,
                          LPCSTR lpDefaultChar,
                          LPBOOL lpUsedDefaultChar);
    HOOK_FUNCTION_DECLARE(UINT, WINAPI, GetKBCodePage);
    HOOK_FUNCTION_DECLARE(LCID, WINAPI, ConvertDefaultLocale, LCID Locale);
    HOOK_FUNCTION_DECLARE(LCID, WINAPI, GetThreadLocale);
    HOOK_FUNCTION_DECLARE(LCID, WINAPI, GetSystemDefaultLCID);
    HOOK_FUNCTION_DECLARE(LCID, WINAPI, GetUserDefaultLCID);
    HOOK_FUNCTION_DECLARE(LANGID, WINAPI, GetSystemDefaultUILanguage);
    HOOK_FUNCTION_DECLARE(LANGID, WINAPI, GetUserDefaultUILanguage);
    HOOK_FUNCTION_DECLARE(LANGID, WINAPI, GetSystemDefaultLangID);
    HOOK_FUNCTION_DECLARE(LANGID, WINAPI, GetUserDefaultLangID);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, GetKeyboardLayoutNameA, LPSTR pwszKLID);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, GetKeyboardLayoutNameW, LPWSTR pwszKLID);
    HOOK_FUNCTION_DECLARE(HKL, WINAPI, GetKeyboardLayout, DWORD idThread);

    HOOK_FUNCTION_DECLARE(int,
                          WINAPI,
                          CompareStringA,
                          LCID Locale,
                          DWORD dwCmpFlags,
                          LPCSTR lpString1,
                          int cchCount1,
                          LPCSTR lpString2,
                          int cchCount2);
    HOOK_FUNCTION_DECLARE(int,
                          WINAPI,
                          CompareStringW,
                          LCID Locale,
                          DWORD dwCmpFlags,
                          LPCWSTR lpString1,
                          int cchCount1,
                          LPCWSTR lpString2,
                          int cchCount2);
    HOOK_FUNCTION_DECLARE(int,
                          WINAPI,
                          GetCalendarInfoA,
                          LCID Locale,
                          CALID Calendar,
                          CALTYPE CalType,
                          LPSTR lpCalData,
                          int cchData,
                          LPDWORD lpValue);
    HOOK_FUNCTION_DECLARE(int,
                          WINAPI,
                          GetCalendarInfoW,
                          LCID Locale,
                          CALID Calendar,
                          CALTYPE CalType,
                          LPWSTR lpCalData,
                          int cchData,
                          LPDWORD lpValue);
    HOOK_FUNCTION_DECLARE(int,
                          WINAPI,
                          GetTimeFormatA,
                          LCID Locale,
                          DWORD dwFlags,
                          CONST SYSTEMTIME* lpTime,
                          LPCSTR lpFormat,
                          LPSTR lpTimeStr,
                          int cchTime);
    HOOK_FUNCTION_DECLARE(int,
                          WINAPI,
                          GetTimeFormatW,
                          LCID Locale,
                          DWORD dwFlags,
                          CONST SYSTEMTIME* lpTime,
                          LPCWSTR lpFormat,
                          LPWSTR lpTimeStr,
                          int cchTime);
    HOOK_FUNCTION_DECLARE(int,
                          WINAPI,
                          GetDateFormatA,
                          LCID Locale,
                          DWORD dwFlags,
                          CONST SYSTEMTIME* lpDate,
                          LPCSTR lpFormat,
                          LPSTR lpDateStr,
                          int cchDate);
    HOOK_FUNCTION_DECLARE(int,
                          WINAPI,
                          GetDateFormatW,
                          LCID Locale,
                          DWORD dwFlags,
                          CONST SYSTEMTIME* lpDate,
                          LPCWSTR lpFormat,
                          LPWSTR lpDateStr,
                          int cchDate);
    HOOK_FUNCTION_DECLARE(int,
                          WINAPI,
                          GetNumberFormatA,
                          LCID Locale,
                          DWORD dwFlags,
                          LPCSTR lpValue,
                          CONST NUMBERFMTA* lpFormat,
                          LPSTR lpNumberStr,
                          int cchNumber);
    HOOK_FUNCTION_DECLARE(int,
                          WINAPI,
                          GetNumberFormatW,
                          LCID Locale,
                          DWORD dwFlags,
                          LPCWSTR lpValue,
                          CONST NUMBERFMTW* lpFormat,
                          LPWSTR lpNumberStr,
                          int cchNumber);
    HOOK_FUNCTION_DECLARE(int,
                          WINAPI,
                          GetCurrencyFormatA,
                          LCID Locale,
                          DWORD dwFlags,
                          LPCSTR lpValue,
                          CONST CURRENCYFMTA* lpFormat,
                          LPSTR lpCurrencyStr,
                          int cchCurrency);
    HOOK_FUNCTION_DECLARE(int,
                          WINAPI,
                          GetCurrencyFormatW,
                          LCID Locale,
                          DWORD dwFlags,
                          LPCWSTR lpValue,
                          CONST CURRENCYFMTW* lpFormat,
                          LPWSTR lpCurrencyStr,
                          int cchCurrency);
    HOOK_FUNCTION_DECLARE(DWORD,
                          WINAPI,
                          GetTimeZoneInformation,
                          LPTIME_ZONE_INFORMATION lpTimeZoneInformation);
    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          SystemParametersInfoA,
                          UINT uiAction,
                          UINT uiParam,
                          PVOID pvParam,
                          UINT fWinIni);
    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          SystemParametersInfoW,
                          UINT uiAction,
                          UINT uiParam,
                          PVOID pvParam,
                          UINT fWinIni);
    //HOOK_FUNCTION_DECLARE(int WINAPI GetTextFaceA(HDC hdc, int c, LPSTR lpName);
    //HOOK_FUNCTION_DECLARE(int WINAPI GetTextFaceW(HDC hdc, int c, LPWSTR lpName);

    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          GetStringTypeA,
                          LCID Locale,
                          DWORD dwInfoType,
                          LPCSTR lpSrcStr,
                          int cchSrc,
                          LPWORD lpCharType);
    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          GetStringTypeW,
                          DWORD dwInfoType,
                          LPCWSTR lpSrcStr,
                          int cchSrc,
                          LPWORD lpCharType);
    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          GetStringTypeExA,
                          LCID Locale,
                          DWORD dwInfoType,
                          LPCSTR lpSrcStr,
                          int cchSrc,
                          LPWORD lpCharType);
    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          GetStringTypeExW,
                          LCID Locale,
                          DWORD dwInfoType,
                          LPCWSTR lpSrcStr,
                          int cchSrc,
                          LPWORD lpCharType);

    HOOK_FUNCTION_DECLARE(VOID, WINAPI, SetFileApisToOEM);
    HOOK_FUNCTION_DECLARE(VOID, WINAPI, SetFileApisToANSI);

    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          TranslateCharsetInfo,
                          DWORD FAR* lpSrc,
                          LPCHARSETINFO lpCs,
                          DWORD dwFlags);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, TextOutA, HDC hdc, int x, int y, LPCSTR lpString, int c);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, TextOutW, HDC hdc, int x, int y, LPCWSTR lpString, int c);
    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          ExtTextOutA,
                          HDC hdc,
                          int x,
                          int y,
                          UINT options,
                          CONST RECT* lprect,
                          LPCSTR lpString,
                          UINT c,
                          CONST INT* lpDx);
    HOOK_FUNCTION_DECLARE(BOOL,
                          WINAPI,
                          ExtTextOutW,
                          HDC hdc,
                          int x,
                          int y,
                          UINT options,
                          CONST RECT* lprect,
                          LPCWSTR lpString,
                          UINT c,
                          CONST INT* lpDx);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, PolyTextOutA, HDC hdc, CONST POLYTEXTA* ppt, int nstrings);
    HOOK_FUNCTION_DECLARE(BOOL, WINAPI, PolyTextOutW, HDC hdc, CONST POLYTEXTW* ppt, int nstrings);

    void ApplyRegistryIntercepts();
}

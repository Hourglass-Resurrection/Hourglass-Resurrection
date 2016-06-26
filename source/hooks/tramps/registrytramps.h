/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#define RegOpenKeyA TrampRegOpenKeyA
TRAMPFUNC LONG APIENTRY RegOpenKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult);
#define RegOpenKeyExA TrampRegOpenKeyExA
TRAMPFUNC LONG APIENTRY RegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
#define RegEnumKeyA TrampRegEnumKeyA
TRAMPFUNC LONG APIENTRY RegEnumKeyA(HKEY hKey, DWORD dwIndex, LPCSTR lpName, DWORD cchName);
#define RegQueryValueExA TrampRegQueryValueExA
TRAMPFUNC LONG APIENTRY RegQueryValueExA(HKEY hKey, LPCTSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
#define RegOpenKeyW TrampRegOpenKeyW
TRAMPFUNC LONG APIENTRY RegOpenKeyW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult);
#define RegOpenKeyExW TrampRegOpenKeyExW
TRAMPFUNC LONG APIENTRY RegOpenKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult);
#define RegEnumKeyW TrampRegEnumKeyW
TRAMPFUNC LONG APIENTRY RegEnumKeyW(HKEY hKey, DWORD dwIndex, LPCWSTR lpName, DWORD cchName);
#define RegQueryValueExW TrampRegQueryValueExW
TRAMPFUNC LONG APIENTRY RegQueryValueExW(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
#define NtQueryKey TrampNtQueryKey
TRAMPFUNC NTSTATUS NTAPI NtQueryKey(IN HANDLE KeyHandle, IN DWORD KeyInformationClass, OUT PVOID KeyInformation, IN ULONG Length, OUT PULONG ResultLength);

// close enough?
#define GetSystemMetrics TrampGetSystemMetrics
TRAMPFUNC int WINAPI GetSystemMetrics(int nIndex);
#define GetACP TrampGetACP
TRAMPFUNC UINT WINAPI GetACP();
#define GetOEMCP TrampGetOEMCP
TRAMPFUNC UINT WINAPI GetOEMCP();
#define GetCPInfo TrampGetCPInfo
TRAMPFUNC BOOL WINAPI GetCPInfo(UINT CodePage, LPCPINFO lpCPInfo);
#define GetCPInfoExA TrampGetCPInfoExA
TRAMPFUNC BOOL WINAPI GetCPInfoExA(UINT CodePage, DWORD dwFlags, LPCPINFOEXA lpCPInfoEx);
#define GetCPInfoExW TrampGetCPInfoExW
TRAMPFUNC BOOL WINAPI GetCPInfoExW(UINT CodePage, DWORD dwFlags, LPCPINFOEXW lpCPInfoEx);
#define GetLocaleInfoA TrampGetLocaleInfoA
TRAMPFUNC int WINAPI GetLocaleInfoA(LCID Locale, LCTYPE LCType, LPSTR lpLCData, int cchData);
#define GetLocaleInfoW TrampGetLocaleInfoW
TRAMPFUNC int WINAPI GetLocaleInfoW(LCID Locale, LCTYPE LCType, LPWSTR lpLCData, int cchData);
#define LCMapStringA TrampLCMapStringA
TRAMPFUNC int WINAPI LCMapStringA(LCID Locale, DWORD dwMapFlags, LPCSTR lpSrcStr, int cchSrc, LPSTR lpDestStr, int cchDest);
#define LCMapStringW TrampLCMapStringW
TRAMPFUNC int WINAPI LCMapStringW(LCID Locale, DWORD dwMapFlags, LPCWSTR lpSrcStr, int cchSrc, LPWSTR lpDestStr, int cchDest);
#define IsDBCSLeadByte TrampIsDBCSLeadByte
TRAMPFUNC BOOL WINAPI IsDBCSLeadByte(BYTE TestChar);
#define IsDBCSLeadByteEx TrampIsDBCSLeadByteEx
TRAMPFUNC BOOL WINAPI IsDBCSLeadByteEx(UINT CodePage, BYTE TestChar);
#define MultiByteToWideChar TrampMultiByteToWideChar
TRAMPFUNC int WINAPI MultiByteToWideChar(UINT CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar);
#define WideCharToMultiByte TrampWideCharToMultiByte
TRAMPFUNC int WINAPI WideCharToMultiByte(UINT CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr, int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte, LPCSTR lpDefaultChar, LPBOOL lpUsedDefaultChar);
#define GetKBCodePage TrampGetKBCodePage
TRAMPFUNC UINT WINAPI GetKBCodePage();
#define ConvertDefaultLocale TrampConvertDefaultLocale
TRAMPFUNC LCID WINAPI ConvertDefaultLocale(LCID Locale);
#define GetThreadLocale TrampGetThreadLocale
TRAMPFUNC LCID WINAPI GetThreadLocale();
#define GetSystemDefaultLCID TrampGetSystemDefaultLCID
TRAMPFUNC LCID WINAPI GetSystemDefaultLCID();
#define GetUserDefaultLCID TrampGetUserDefaultLCID
TRAMPFUNC LCID WINAPI GetUserDefaultLCID();
#define GetSystemDefaultUILanguage TrampGetSystemDefaultUILanguage
TRAMPFUNC LANGID WINAPI GetSystemDefaultUILanguage();
#define GetUserDefaultUILanguage TrampGetUserDefaultUILanguage
TRAMPFUNC LANGID WINAPI GetUserDefaultUILanguage();
#define GetSystemDefaultLangID TrampGetSystemDefaultLangID
TRAMPFUNC LANGID WINAPI GetSystemDefaultLangID();
#define GetUserDefaultLangID TrampGetUserDefaultLangID
TRAMPFUNC LANGID WINAPI GetUserDefaultLangID();
#define GetKeyboardLayoutNameA TrampGetKeyboardLayoutNameA
TRAMPFUNC BOOL WINAPI GetKeyboardLayoutNameA(LPSTR pwszKLID);
#define GetKeyboardLayoutNameW TrampGetKeyboardLayoutNameW
TRAMPFUNC BOOL WINAPI GetKeyboardLayoutNameW(LPWSTR pwszKLID);
#define GetKeyboardLayout TrampGetKeyboardLayout
TRAMPFUNC HKL WINAPI GetKeyboardLayout(DWORD idThread);

#define CompareStringA TrampCompareStringA
#define CompareStringW TrampCompareStringW
#define GetCalendarInfoA TrampGetCalendarInfoA
#define GetCalendarInfoW TrampGetCalendarInfoW
#define GetTimeFormatA TrampGetTimeFormatA
#define GetTimeFormatW TrampGetTimeFormatW
#define GetDateFormatA TrampGetDateFormatA
#define GetDateFormatW TrampGetDateFormatW
#define GetNumberFormatA TrampGetNumberFormatA
#define GetNumberFormatW TrampGetNumberFormatW
#define GetCurrencyFormatA TrampGetCurrencyFormatA
#define GetCurrencyFormatW TrampGetCurrencyFormatW
#define GetTimeZoneInformation TrampGetTimeZoneInformation
#define SystemParametersInfoA TrampSystemParametersInfoA
#define SystemParametersInfoW TrampSystemParametersInfoW
//#define GetTextFaceA TrampGetTextFaceA
//#define GetTextFaceW TrampGetTextFaceW
TRAMPFUNC int WINAPI CompareStringA(LCID Locale, DWORD dwCmpFlags, LPCSTR lpString1, int cchCount1, LPCSTR lpString2, int cchCount2);
TRAMPFUNC int WINAPI CompareStringW(LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2);
TRAMPFUNC int WINAPI GetCalendarInfoA(LCID Locale, CALID Calendar, CALTYPE CalType, LPSTR lpCalData, int cchData, LPDWORD lpValue); 
TRAMPFUNC int WINAPI GetCalendarInfoW(LCID Locale, CALID Calendar, CALTYPE CalType, LPWSTR lpCalData, int cchData, LPDWORD lpValue);
TRAMPFUNC int WINAPI GetTimeFormatA(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME *lpTime, LPCSTR lpFormat, LPSTR lpTimeStr, int cchTime);
TRAMPFUNC int WINAPI GetTimeFormatW(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME *lpTime, LPCWSTR lpFormat, LPWSTR lpTimeStr, int cchTime);
TRAMPFUNC int WINAPI GetDateFormatA(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME *lpDate, LPCSTR lpFormat, LPSTR lpDateStr, int cchDate);
TRAMPFUNC int WINAPI GetDateFormatW(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME *lpDate, LPCWSTR lpFormat, LPWSTR lpDateStr, int cchDate);
TRAMPFUNC int WINAPI GetNumberFormatA(LCID Locale, DWORD dwFlags, LPCSTR lpValue, CONST NUMBERFMTA *lpFormat, LPSTR lpNumberStr, int cchNumber);
TRAMPFUNC int WINAPI GetNumberFormatW(LCID Locale, DWORD dwFlags, LPCWSTR lpValue, CONST NUMBERFMTW *lpFormat, LPWSTR lpNumberStr, int cchNumber);
TRAMPFUNC int WINAPI GetCurrencyFormatA(LCID Locale, DWORD dwFlags, LPCSTR lpValue, CONST CURRENCYFMTA *lpFormat, LPSTR lpCurrencyStr, int cchCurrency);
TRAMPFUNC int WINAPI GetCurrencyFormatW(LCID Locale, DWORD dwFlags, LPCWSTR lpValue, CONST CURRENCYFMTW *lpFormat, LPWSTR lpCurrencyStr, int cchCurrency);
TRAMPFUNC DWORD WINAPI GetTimeZoneInformation(LPTIME_ZONE_INFORMATION lpTimeZoneInformation);
TRAMPFUNC BOOL WINAPI SystemParametersInfoA(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni);
TRAMPFUNC BOOL WINAPI SystemParametersInfoW(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni);
//TRAMPFUNC int WINAPI GetTextFaceA(HDC hdc, int c, LPSTR lpName);
//TRAMPFUNC int WINAPI GetTextFaceW(HDC hdc, int c, LPWSTR lpName);

#define GetStringTypeA TrampGetStringTypeA
#define GetStringTypeW TrampGetStringTypeW
#define GetStringTypeExA TrampGetStringTypeExA
#define GetStringTypeExW TrampGetStringTypeExW
TRAMPFUNC BOOL WINAPI GetStringTypeA(LCID Locale, DWORD dwInfoType, LPCSTR lpSrcStr, int cchSrc, LPWORD lpCharType); 
TRAMPFUNC BOOL WINAPI GetStringTypeW(DWORD dwInfoType, LPCWSTR lpSrcStr, int cchSrc, LPWORD lpCharType); 
TRAMPFUNC BOOL WINAPI GetStringTypeExA(LCID Locale, DWORD dwInfoType, LPCSTR lpSrcStr, int cchSrc, LPWORD lpCharType); 
TRAMPFUNC BOOL WINAPI GetStringTypeExW(LCID Locale, DWORD dwInfoType, LPCWSTR lpSrcStr, int cchSrc, LPWORD lpCharType); 

#define SetFileApisToOEM TrampSetFileApisToOEM
#define SetFileApisToANSI TrampSetFileApisToANSI
TRAMPFUNC VOID WINAPI SetFileApisToOEM();
TRAMPFUNC VOID WINAPI SetFileApisToANSI();


#define TranslateCharsetInfo TrampTranslateCharsetInfo
TRAMPFUNC BOOL WINAPI TranslateCharsetInfo(DWORD FAR *lpSrc, LPCHARSETINFO lpCs, DWORD dwFlags);
#define TextOutA TrampTextOutA
TRAMPFUNC BOOL WINAPI TextOutA(HDC hdc, int x, int y, LPCSTR lpString, int c);
#define TextOutW TrampTextOutW
TRAMPFUNC BOOL WINAPI TextOutW(HDC hdc, int x, int y, LPCWSTR lpString, int c);
#define ExtTextOutA TrampExtTextOutA
TRAMPFUNC BOOL WINAPI ExtTextOutA(HDC hdc, int x, int y, UINT options, CONST RECT * lprect, LPCSTR lpString, UINT c, CONST INT * lpDx);
#define ExtTextOutW TrampExtTextOutW
TRAMPFUNC BOOL WINAPI ExtTextOutW(HDC hdc, int x, int y, UINT options, CONST RECT * lprect, LPCWSTR lpString, UINT c, CONST INT * lpDx);
#define PolyTextOutA TrampPolyTextOutA
TRAMPFUNC BOOL WINAPI PolyTextOutA(HDC hdc, CONST POLYTEXTA * ppt, int nstrings);
#define PolyTextOutW TrampPolyTextOutW
TRAMPFUNC BOOL WINAPI PolyTextOutW(HDC hdc, CONST POLYTEXTW * ppt, int nstrings);

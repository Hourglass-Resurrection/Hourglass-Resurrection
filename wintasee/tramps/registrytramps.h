/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef REGISTRYTRAMPS_H_INCL
#define REGISTRYTRAMPS_H_INCL

#define RegOpenKeyA TrampRegOpenKeyA
TRAMPFUNC LONG APIENTRY RegOpenKeyA(HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult) TRAMPOLINE_DEF
#define RegOpenKeyExA TrampRegOpenKeyExA
TRAMPFUNC LONG APIENTRY RegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult) TRAMPOLINE_DEF
#define RegEnumKeyA TrampRegEnumKeyA
TRAMPFUNC LONG APIENTRY RegEnumKeyA(HKEY hKey, DWORD dwIndex, LPCSTR lpName, DWORD cchName) TRAMPOLINE_DEF
#define RegQueryValueExA TrampRegQueryValueExA
TRAMPFUNC LONG APIENTRY RegQueryValueExA(HKEY hKey, LPCTSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) TRAMPOLINE_DEF
#define RegOpenKeyW TrampRegOpenKeyW
TRAMPFUNC LONG APIENTRY RegOpenKeyW(HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult) TRAMPOLINE_DEF
#define RegOpenKeyExW TrampRegOpenKeyExW
TRAMPFUNC LONG APIENTRY RegOpenKeyExW(HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult) TRAMPOLINE_DEF
#define RegEnumKeyW TrampRegEnumKeyW
TRAMPFUNC LONG APIENTRY RegEnumKeyW(HKEY hKey, DWORD dwIndex, LPCWSTR lpName, DWORD cchName) TRAMPOLINE_DEF
#define RegQueryValueExW TrampRegQueryValueExW
TRAMPFUNC LONG APIENTRY RegQueryValueExW(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData) TRAMPOLINE_DEF
#define NtQueryKey TrampNtQueryKey
TRAMPFUNC NTSTATUS NTAPI NtQueryKey(IN HANDLE KeyHandle, IN DWORD KeyInformationClass, OUT PVOID KeyInformation, IN ULONG Length, OUT PULONG ResultLength) TRAMPOLINE_DEF

// close enough?
#define GetSystemMetrics TrampGetSystemMetrics
TRAMPFUNC int WINAPI GetSystemMetrics(int nIndex) TRAMPOLINE_DEF
#define GetACP TrampGetACP
TRAMPFUNC UINT WINAPI GetACP() TRAMPOLINE_DEF
#define GetOEMCP TrampGetOEMCP
TRAMPFUNC UINT WINAPI GetOEMCP() TRAMPOLINE_DEF
#define GetCPInfo TrampGetCPInfo
TRAMPFUNC BOOL WINAPI GetCPInfo(UINT CodePage, LPCPINFO lpCPInfo) TRAMPOLINE_DEF
#define GetCPInfoExA TrampGetCPInfoExA
TRAMPFUNC BOOL WINAPI GetCPInfoExA(UINT CodePage, DWORD dwFlags, LPCPINFOEXA lpCPInfoEx) TRAMPOLINE_DEF
#define GetCPInfoExW TrampGetCPInfoExW
TRAMPFUNC BOOL WINAPI GetCPInfoExW(UINT CodePage, DWORD dwFlags, LPCPINFOEXW lpCPInfoEx) TRAMPOLINE_DEF
#define GetLocaleInfoA TrampGetLocaleInfoA
TRAMPFUNC int WINAPI GetLocaleInfoA(LCID Locale, LCTYPE LCType, LPSTR lpLCData, int cchData) TRAMPOLINE_DEF
#define GetLocaleInfoW TrampGetLocaleInfoW
TRAMPFUNC int WINAPI GetLocaleInfoW(LCID Locale, LCTYPE LCType, LPWSTR lpLCData, int cchData) TRAMPOLINE_DEF
#define LCMapStringA TrampLCMapStringA
TRAMPFUNC int WINAPI LCMapStringA(LCID Locale, DWORD dwMapFlags, LPCSTR lpSrcStr, int cchSrc, LPSTR lpDestStr, int cchDest) TRAMPOLINE_DEF
#define LCMapStringW TrampLCMapStringW
TRAMPFUNC int WINAPI LCMapStringW(LCID Locale, DWORD dwMapFlags, LPCWSTR lpSrcStr, int cchSrc, LPWSTR lpDestStr, int cchDest) TRAMPOLINE_DEF
#define IsDBCSLeadByte TrampIsDBCSLeadByte
TRAMPFUNC BOOL WINAPI IsDBCSLeadByte(BYTE TestChar) TRAMPOLINE_DEF
#define IsDBCSLeadByteEx TrampIsDBCSLeadByteEx
TRAMPFUNC BOOL WINAPI IsDBCSLeadByteEx(UINT CodePage, BYTE TestChar) TRAMPOLINE_DEF
#define MultiByteToWideChar TrampMultiByteToWideChar
TRAMPFUNC int WINAPI MultiByteToWideChar(UINT CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar) TRAMPOLINE_DEF
#define WideCharToMultiByte TrampWideCharToMultiByte
TRAMPFUNC int WINAPI WideCharToMultiByte(UINT CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr, int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte, LPCSTR lpDefaultChar, LPBOOL lpUsedDefaultChar) TRAMPOLINE_DEF
#define GetKBCodePage TrampGetKBCodePage
TRAMPFUNC UINT WINAPI GetKBCodePage() TRAMPOLINE_DEF
#define ConvertDefaultLocale TrampConvertDefaultLocale
TRAMPFUNC LCID WINAPI ConvertDefaultLocale(LCID Locale) TRAMPOLINE_DEF
#define GetThreadLocale TrampGetThreadLocale
TRAMPFUNC LCID WINAPI GetThreadLocale() TRAMPOLINE_DEF
#define GetSystemDefaultLCID TrampGetSystemDefaultLCID
TRAMPFUNC LCID WINAPI GetSystemDefaultLCID() TRAMPOLINE_DEF
#define GetUserDefaultLCID TrampGetUserDefaultLCID
TRAMPFUNC LCID WINAPI GetUserDefaultLCID() TRAMPOLINE_DEF
#define GetSystemDefaultUILanguage TrampGetSystemDefaultUILanguage
TRAMPFUNC LANGID WINAPI GetSystemDefaultUILanguage() TRAMPOLINE_DEF
#define GetUserDefaultUILanguage TrampGetUserDefaultUILanguage
TRAMPFUNC LANGID WINAPI GetUserDefaultUILanguage() TRAMPOLINE_DEF
#define GetSystemDefaultLangID TrampGetSystemDefaultLangID
TRAMPFUNC LANGID WINAPI GetSystemDefaultLangID() TRAMPOLINE_DEF
#define GetUserDefaultLangID TrampGetUserDefaultLangID
TRAMPFUNC LANGID WINAPI GetUserDefaultLangID() TRAMPOLINE_DEF
#define GetKeyboardLayoutNameA TrampGetKeyboardLayoutNameA
TRAMPFUNC BOOL WINAPI GetKeyboardLayoutNameA(LPSTR pwszKLID) TRAMPOLINE_DEF
#define GetKeyboardLayoutNameW TrampGetKeyboardLayoutNameW
TRAMPFUNC BOOL WINAPI GetKeyboardLayoutNameW(LPWSTR pwszKLID) TRAMPOLINE_DEF
#define GetKeyboardLayout TrampGetKeyboardLayout
TRAMPFUNC HKL WINAPI GetKeyboardLayout(DWORD idThread) TRAMPOLINE_DEF

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
TRAMPFUNC int WINAPI CompareStringA(LCID Locale, DWORD dwCmpFlags, LPCSTR lpString1, int cchCount1, LPCSTR lpString2, int cchCount2) TRAMPOLINE_DEF
TRAMPFUNC int WINAPI CompareStringW(LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2) TRAMPOLINE_DEF
TRAMPFUNC int WINAPI GetCalendarInfoA(LCID Locale, CALID Calendar, CALTYPE CalType, LPSTR lpCalData, int cchData, LPDWORD lpValue) TRAMPOLINE_DEF 
TRAMPFUNC int WINAPI GetCalendarInfoW(LCID Locale, CALID Calendar, CALTYPE CalType, LPWSTR lpCalData, int cchData, LPDWORD lpValue) TRAMPOLINE_DEF
TRAMPFUNC int WINAPI GetTimeFormatA(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME *lpTime, LPCSTR lpFormat, LPSTR lpTimeStr, int cchTime) TRAMPOLINE_DEF
TRAMPFUNC int WINAPI GetTimeFormatW(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME *lpTime, LPCWSTR lpFormat, LPWSTR lpTimeStr, int cchTime) TRAMPOLINE_DEF
TRAMPFUNC int WINAPI GetDateFormatA(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME *lpDate, LPCSTR lpFormat, LPSTR lpDateStr, int cchDate) TRAMPOLINE_DEF
TRAMPFUNC int WINAPI GetDateFormatW(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME *lpDate, LPCWSTR lpFormat, LPWSTR lpDateStr, int cchDate) TRAMPOLINE_DEF
TRAMPFUNC int WINAPI GetNumberFormatA(LCID Locale, DWORD dwFlags, LPCSTR lpValue, CONST NUMBERFMTA *lpFormat, LPSTR lpNumberStr, int cchNumber) TRAMPOLINE_DEF
TRAMPFUNC int WINAPI GetNumberFormatW(LCID Locale, DWORD dwFlags, LPCWSTR lpValue, CONST NUMBERFMTW *lpFormat, LPWSTR lpNumberStr, int cchNumber) TRAMPOLINE_DEF
TRAMPFUNC int WINAPI GetCurrencyFormatA(LCID Locale, DWORD dwFlags, LPCSTR lpValue, CONST CURRENCYFMTA *lpFormat, LPSTR lpCurrencyStr, int cchCurrency) TRAMPOLINE_DEF
TRAMPFUNC int WINAPI GetCurrencyFormatW(LCID Locale, DWORD dwFlags, LPCWSTR lpValue, CONST CURRENCYFMTW *lpFormat, LPWSTR lpCurrencyStr, int cchCurrency) TRAMPOLINE_DEF
TRAMPFUNC DWORD WINAPI GetTimeZoneInformation(LPTIME_ZONE_INFORMATION lpTimeZoneInformation) TRAMPOLINE_DEF
TRAMPFUNC BOOL WINAPI SystemParametersInfoA(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni) TRAMPOLINE_DEF
TRAMPFUNC BOOL WINAPI SystemParametersInfoW(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni) TRAMPOLINE_DEF
//TRAMPFUNC int WINAPI GetTextFaceA(HDC hdc, int c, LPSTR lpName) TRAMPOLINE_DEF
//TRAMPFUNC int WINAPI GetTextFaceW(HDC hdc, int c, LPWSTR lpName) TRAMPOLINE_DEF

#define GetStringTypeA TrampGetStringTypeA
#define GetStringTypeW TrampGetStringTypeW
#define GetStringTypeExA TrampGetStringTypeExA
#define GetStringTypeExW TrampGetStringTypeExW
TRAMPFUNC BOOL WINAPI GetStringTypeA(LCID Locale, DWORD dwInfoType, LPCSTR lpSrcStr, int cchSrc, LPWORD lpCharType) TRAMPOLINE_DEF 
TRAMPFUNC BOOL WINAPI GetStringTypeW(DWORD dwInfoType, LPCWSTR lpSrcStr, int cchSrc, LPWORD lpCharType) TRAMPOLINE_DEF 
TRAMPFUNC BOOL WINAPI GetStringTypeExA(LCID Locale, DWORD dwInfoType, LPCSTR lpSrcStr, int cchSrc, LPWORD lpCharType) TRAMPOLINE_DEF 
TRAMPFUNC BOOL WINAPI GetStringTypeExW(LCID Locale, DWORD dwInfoType, LPCWSTR lpSrcStr, int cchSrc, LPWORD lpCharType) TRAMPOLINE_DEF 

#define SetFileApisToOEM TrampSetFileApisToOEM
#define SetFileApisToANSI TrampSetFileApisToANSI
TRAMPFUNC VOID WINAPI SetFileApisToOEM() TRAMPOLINE_DEF_VOID
TRAMPFUNC VOID WINAPI SetFileApisToANSI() TRAMPOLINE_DEF_VOID


#define TranslateCharsetInfo TrampTranslateCharsetInfo
TRAMPFUNC BOOL WINAPI TranslateCharsetInfo(DWORD FAR *lpSrc, LPCHARSETINFO lpCs, DWORD dwFlags) TRAMPOLINE_DEF
#define TextOutA TrampTextOutA
TRAMPFUNC BOOL WINAPI TextOutA(HDC hdc, int x, int y, LPCSTR lpString, int c) TRAMPOLINE_DEF
#define TextOutW TrampTextOutW
TRAMPFUNC BOOL WINAPI TextOutW(HDC hdc, int x, int y, LPCWSTR lpString, int c) TRAMPOLINE_DEF
#define ExtTextOutA TrampExtTextOutA
TRAMPFUNC BOOL WINAPI ExtTextOutA(HDC hdc, int x, int y, UINT options, CONST RECT * lprect, LPCSTR lpString, UINT c, CONST INT * lpDx) TRAMPOLINE_DEF
#define ExtTextOutW TrampExtTextOutW
TRAMPFUNC BOOL WINAPI ExtTextOutW(HDC hdc, int x, int y, UINT options, CONST RECT * lprect, LPCWSTR lpString, UINT c, CONST INT * lpDx) TRAMPOLINE_DEF
#define PolyTextOutA TrampPolyTextOutA
TRAMPFUNC BOOL WINAPI PolyTextOutA(HDC hdc, CONST POLYTEXTA * ppt, int nstrings) TRAMPOLINE_DEF
#define PolyTextOutW TrampPolyTextOutW
TRAMPFUNC BOOL WINAPI PolyTextOutW(HDC hdc, CONST POLYTEXTW * ppt, int nstrings) TRAMPOLINE_DEF


#endif

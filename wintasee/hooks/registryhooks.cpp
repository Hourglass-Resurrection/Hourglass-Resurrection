/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(REGISTRYHOOKS_INCL) && !defined(UNITY_BUILD)
#define REGISTRYHOOKS_INCL


#include "../global.h"
#include "../../shared/ipc.h"
#include "../tls.h"
#include "../wintasee.h"
#include "../locale.h"
#include <memory>

typedef struct _KEY_NAME_INFORMATION {
  ULONG NameLength;
  WCHAR Name[1];
} KEY_NAME_INFORMATION, *PKEY_NAME_INFORMATION;
//NTSYSAPI NTSTATUS NTAPI NtQueryKey(IN HANDLE KeyHandle, IN DWORD KeyInformationClass, OUT PVOID KeyInformation, IN ULONG Length, OUT PULONG ResultLength);

static const char* RegistryKeyToName(HKEY hKey)
{
	switch((LONG)(ULONG_PTR)hKey)
	{
	case 0: return "NULL";
	case HKEY_CLASSES_ROOT: return "HKEY_CLASSES_ROOT";
	case HKEY_CURRENT_USER: return "HKEY_CURRENT_USER";
	case HKEY_LOCAL_MACHINE: return "HKEY_LOCAL_MACHINE";
	case HKEY_USERS: return "HKEY_USERS";
	case HKEY_PERFORMANCE_DATA: return "HKEY_PERFORMANCE_DATA";
	case HKEY_PERFORMANCE_TEXT: return "HKEY_PERFORMANCE_TEXT";
	case HKEY_PERFORMANCE_NLSTEXT: return "HKEY_PERFORMANCE_NLSTEXT";
	case HKEY_CURRENT_CONFIG: return "HKEY_CURRENT_CONFIG";
	case HKEY_DYN_DATA: return "HKEY_DYN_DATA";
	default:
		{
			//return "?";
			static const int MAXLEN = 512;
			char basicInfoBytes [sizeof(KEY_NAME_INFORMATION) + (MAXLEN-1)*sizeof(WCHAR)] = {0};
			ULONG resultLength = 0;
			NtQueryKey(hKey, /*KeyNameInformation*/3, basicInfoBytes, MAXLEN, &resultLength);
			int length = (int)(((KEY_NAME_INFORMATION*)basicInfoBytes)->NameLength);
			length = min(length, (int)resultLength);
			length = min(length, MAXLEN);
			static char rvtemp [MAXLEN];
			rvtemp[sizeof(rvtemp)-1] = 0;
			char* rvtempPtr = rvtemp;
			WCHAR* wname = ((KEY_NAME_INFORMATION*)basicInfoBytes)->Name;
			do{
				*rvtempPtr++ = (char)*wname++;
			} while(rvtempPtr[-1]);
			return rvtemp;
		}
	}
} 

HOOKFUNC LONG APIENTRY MyRegOpenKeyA(
HKEY hKey, LPCSTR lpSubKey, PHKEY phkResult)
{
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__ "(\"%s\", \"%s\") called.\n", RegistryKeyToName(hKey), lpSubKey ? lpSubKey : "");
	LONG rv = RegOpenKeyA(hKey, lpSubKey, phkResult);
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__" returned \"%s\".\n", RegistryKeyToName(phkResult ? *phkResult : 0));
	return rv;
}
HOOKFUNC LONG APIENTRY MyRegOpenKeyW(
HKEY hKey, LPCWSTR lpSubKey, PHKEY phkResult)
{
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__ "(\"%s\", \"%S\") called.\n", RegistryKeyToName(hKey), lpSubKey ? lpSubKey : L"");
	LONG rv = RegOpenKeyW(hKey, lpSubKey, phkResult);
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__" returned \"%s\".\n", RegistryKeyToName(phkResult ? *phkResult : 0));
	return rv;
}
HOOKFUNC LONG APIENTRY MyRegOpenKeyExA(
HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
{
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__ "(\"%s\", \"%s\") called.\n", RegistryKeyToName(hKey), lpSubKey ? lpSubKey : "");
	LONG rv = RegOpenKeyExA(hKey, lpSubKey, ulOptions, samDesired, phkResult);
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__" returned \"%s\".\n", RegistryKeyToName(phkResult ? *phkResult : 0));
	return rv;
}
HOOKFUNC LONG APIENTRY MyRegOpenKeyExW(
HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
{
	debuglog(LCF_REGISTRY|LCF_TODO|LCF_FREQUENT, __FUNCTION__ "(\"%s\", \"%S\") called.\n", RegistryKeyToName(hKey), lpSubKey ? lpSubKey : L"");
	LONG rv = RegOpenKeyExW(hKey, lpSubKey, ulOptions, samDesired, phkResult);
	debuglog(LCF_REGISTRY|LCF_TODO|LCF_FREQUENT, __FUNCTION__" returned \"%s\".\n", RegistryKeyToName(phkResult ? *phkResult : 0));
	return rv;
}
HOOKFUNC LONG APIENTRY MyRegEnumKeyA(HKEY hKey, DWORD dwIndex, LPCSTR lpName, DWORD cchName)
{
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__ "(\"%s\", %d) called.\n", RegistryKeyToName(hKey), dwIndex);
	LONG rv = RegEnumKeyA(hKey, dwIndex, lpName, cchName);
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__" returned \"%s\".\n", lpName);
	return rv;
}
HOOKFUNC LONG APIENTRY MyRegEnumKeyW(HKEY hKey, DWORD dwIndex, LPCWSTR lpName, DWORD cchName)
{
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__ "(\"%s\", %d) called.\n", RegistryKeyToName(hKey), dwIndex);
	LONG rv = RegEnumKeyW(hKey, dwIndex, lpName, cchName);
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__" returned \"%S\".\n", lpName);
	return rv;
}
HOOKFUNC LONG APIENTRY MyRegQueryValueExA(HKEY hKey, LPCSTR lpValueName, LPDWORD lpReserved,
													 LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__ "(\"%s\", \"%s\") called.\n", RegistryKeyToName(hKey), lpValueName ? lpValueName : "");
	LONG rv = RegQueryValueExA(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__" returned \"%s\".\n", lpValueName, lpData); // FIXME: lpData isn't always a string
	if(tasflags.forceSoftware && lpValueName && !strcmp(lpValueName, "EmulationOnly"))
		lpData[0] = 1; // disable hardware acceleration, maybe not needed anymore but it gets rid of some savestate error messages
	//if(lpValueName && !strcmp(lpValueName, "FontSmoothing"))
	//	lpData[0] = 0;
	//if(lpValueName && !strcmp(lpValueName, "FontSmoothingType"))
	//	lpData[0] = 0;
	//if(lpValueName && !strcmp(lpValueName, "ClearTypeLevel"))
	//	lpData[0] = 0;
	return rv;
}

HOOKFUNC LONG APIENTRY MyRegQueryValueExW(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved,
													 LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData)
{
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__ "(\"%s\", \"%S\") called.\n", RegistryKeyToName(hKey), lpValueName ? lpValueName : L"");
	LONG rv = RegQueryValueExW(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
	debuglog(LCF_REGISTRY|LCF_TODO, __FUNCTION__" returned \"%s\".\n", lpValueName, lpData);
	if(tasflags.forceSoftware && lpValueName && !wcscmp(lpValueName, L"EmulationOnly"))
		lpData[0] = 1; // disable hardware acceleration, maybe not needed anymore but it gets rid of some savestate error messages
	//if(lpValueName && !wcscmp(lpValueName, L"FontSmoothing"))
	//	lpData[0] = 0;
	//if(lpValueName && !wcscmp(lpValueName, L"FontSmoothingType"))
	//	lpData[0] = 0;
	//if(lpValueName && !wcscmp(lpValueName, L"ClearTypeLevel"))
	//	lpData[0] = 0;
	return rv;
}

HOOKFUNC NTSTATUS NTAPI MyNtQueryKey(IN HANDLE KeyHandle, IN DWORD KeyInformationClass, OUT PVOID KeyInformation, IN ULONG Length, OUT PULONG ResultLength)
{
	return NtQueryKey(KeyHandle, KeyInformationClass, KeyInformation, Length, ResultLength);
}


HOOKFUNC int WINAPI MyGetSystemMetrics(int nIndex)
{
	ThreadLocalStuff& curtls = tls;
	curtls.callerisuntrusted++;
	curtls.treatDLLLoadsAsClient++;
	int systemResult = GetSystemMetrics(nIndex);
	curtls.treatDLLLoadsAsClient--;
	curtls.callerisuntrusted--;
	int rv = systemResult;
	switch(nIndex)
	{
	case SM_CLEANBOOT:
		rv = 0; // normal boot
		break;
	case SM_CMOUSEBUTTONS:
		rv = 8; // upper limit... any more would take a new byte per frame to record anyway
		break;
	case SM_CXDOUBLECLK:
		rv = 4;
		break;
	case SM_CXDRAG:
		rv = 4;
		break;
	case SM_DBCSENABLED:
	case SM_MIDEASTENABLED:
	case SM_IMMENABLED:
		rv = 1;
		break;
	case /*SM_DIGITIZER*/94:
	case /*SM_MAXIMUMTOUCHES*/95:
	case /*SM_MEDIACENTER*/87:
	case SM_DEBUG:
	case SM_MENUDROPALIGNMENT:
	case SM_NETWORK:
	case SM_PENWINDOWS:
	case SM_REMOTECONTROL:
	case SM_REMOTESESSION:
	case /*SM_SERVERR2*/89:
	case SM_SHOWSOUNDS:
	case /*SM_SHUTTINGDOWN*/0x2000:
	case SM_SLOWMACHINE:
	case /*SM_STARTER*/88:
	case /*SM_TABLETPC*/86:
		rv = 0; // disabled unnecessary things
		break;
	case SM_MOUSEPRESENT:
		rv = 1; // "This value is rarely zero"
		break;
	case /*SM_MOUSEHORIZONTALWHEELPRESENT*/91:
	case SM_MOUSEWHEELPRESENT:
	case SM_SWAPBUTTON:
		rv = 0; // FIXME: TODO: customize some of these depending on mouse settings
		break;
	case SM_CMONITORS:
		if(tasflags.forceWindowed)
			rv = 1;
		// else allow the system setting to be used here
	case SM_CXSCREEN:
	case SM_CXFULLSCREEN:
	case SM_CXVIRTUALSCREEN:
		if(fakeDisplayValid)
			rv = fakeDisplayWidth;
		// else allow the system setting to be used here
		break;
	case SM_CYSCREEN:
	case SM_CYFULLSCREEN:
	case SM_CYVIRTUALSCREEN:
		if(fakeDisplayValid)
			rv = fakeDisplayHeight;
		// else allow the system setting to be used here
		break;
	case SM_SAMEDISPLAYFORMAT:
		if(tasflags.forceWindowed)
			rv = 1;
		// else allow the system setting to be used here
		break;
	case SM_ARRANGE:
	case SM_CXBORDER:
	case SM_CYBORDER:
	case SM_CXCURSOR:
	case SM_CYCURSOR:
	case SM_CXDLGFRAME:
	case SM_CYDLGFRAME:
	case SM_CXEDGE:
	case SM_CYEDGE:
	case /*SM_CXFOCUSBORDER*/83:
	case /*SM_CYFOCUSBORDER*/84:
	case SM_CXFRAME:
	case SM_CYFRAME:
	case SM_CXHSCROLL:
	case SM_CYHSCROLL:
	case SM_CXHTHUMB:
	case SM_CYVTHUMB:
	case SM_CXICON:
	case SM_CYICON:
	case SM_CXICONSPACING:
	case SM_CYICONSPACING:
	case SM_CXMAXIMIZED:
	case SM_CYMAXIMIZED:
	case SM_CXMAXTRACK:
	case SM_CYMAXTRACK:
	case SM_CXMENUCHECK:
	case SM_CYMENUCHECK:
	case SM_CXMENUSIZE:
	case SM_CYMENUSIZE:
	case SM_CXMIN:
	case SM_CYMIN:
	case SM_CXMINIMIZED:
	case SM_CYMINIMIZED:
	case SM_CXMINSPACING:
	case SM_CYMINSPACING:
	case SM_CXMINTRACK:
	case SM_CYMINTRACK:
	case /*SM_CXPADDEDBORDER*/92:
	case SM_CXSIZE:
	case SM_CYSIZE:
	case SM_CXSMICON:
	case SM_CYSMICON:
	case SM_CXSMSIZE:
	case SM_CYSMSIZE:
	case SM_CXVSCROLL:
	case SM_CYVSCROLL:
	case SM_XVIRTUALSCREEN:
	case SM_YVIRTUALSCREEN:
	default:
		// allow system setting
		// hopefully these aren't used in sync-affecting ways...
		// they easily could be, but the idea here is that
		// I don't want to force everyone to use the same desktop resolution or window positions,
		// and most games don't have any game logic affected by those things,
		// and if they did then this definitely isn't the only place that needs to change to handle it.
		// TODO: probably some of these will need to be handled eventually to avoid desyncs in some games.
		break;
	}
	if(rv != systemResult)
		debuglog(LCF_REGISTRY|LCF_TODO|LCF_FREQUENT, __FUNCTION__ "(%d) returned %d (0x%X) instead of %d (0x%X).\n", nIndex, rv, rv, systemResult, systemResult);
	else
		debuglog(LCF_REGISTRY|LCF_TODO|LCF_FREQUENT, __FUNCTION__ "(%d) returned %d (0x%X).\n", nIndex, rv, rv);
	return rv;
}

UINT LocaleToCodePage(LCID locale)
{
	switch(locale)
	{
		case 1041: return 932; // shift-JIS
		case 2052: return 936; // simplified chinese
		case 1042: return 949; // korean
		default: // most others NYI
		case 1033: return 1252;
	}
}
DWORD LocaleToCharset(LCID locale)
{
	switch(locale)
	{
		case 1041: return SHIFTJIS_CHARSET;
		case 2052: return GB2312_CHARSET;
		case 1042: return HANGUL_CHARSET;
		default: // most others NYI
		case 1033: return ANSI_CHARSET;
	}
}


HOOKFUNC UINT WINAPI MyGetACP()
{
	debuglog(LCF_REGISTRY, __FUNCTION__ " called.\n");
	if(tasflags.appLocale)
		return LocaleToCodePage(tasflags.appLocale);
	return GetACP();
}
HOOKFUNC UINT WINAPI MyGetOEMCP()
{
	debuglog(LCF_REGISTRY, __FUNCTION__ " called.\n");
	if(tasflags.appLocale)
		return LocaleToCodePage(tasflags.appLocale);
	return GetOEMCP();
}
HOOKFUNC BOOL WINAPI MyGetCPInfo(UINT CodePage, LPCPINFO lpCPInfo)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d) called.\n", CodePage);
	if(tasflags.appLocale && (CodePage == CP_ACP || CodePage == CP_OEMCP || CodePage == CP_THREAD_ACP || tls.forceLocale))
		CodePage = LocaleToCodePage(tasflags.appLocale);
	return GetCPInfo(CodePage, lpCPInfo);
}
HOOKFUNC BOOL WINAPI MyGetCPInfoExA(UINT CodePage, DWORD dwFlags, LPCPINFOEXA lpCPInfoEx)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d) called.\n", CodePage);
	if(tasflags.appLocale && (CodePage == CP_ACP || CodePage == CP_OEMCP || CodePage == CP_THREAD_ACP || tls.forceLocale))
		CodePage = LocaleToCodePage(tasflags.appLocale);
	return GetCPInfoExA(CodePage, dwFlags, lpCPInfoEx);
}
HOOKFUNC BOOL WINAPI MyGetCPInfoExW(UINT CodePage, DWORD dwFlags, LPCPINFOEXW lpCPInfoEx)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d) called.\n", CodePage);
	if(tasflags.appLocale && (CodePage == CP_ACP || CodePage == CP_OEMCP || CodePage == CP_THREAD_ACP || tls.forceLocale))
		CodePage = LocaleToCodePage(tasflags.appLocale);
	return GetCPInfoExW(CodePage, dwFlags, lpCPInfoEx);
}
HOOKFUNC BOOL WINAPI MyIsDBCSLeadByte(BYTE TestChar)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ " called.\n");
	if(tasflags.appLocale)
		return IsDBCSLeadByteEx(LocaleToCodePage(tasflags.appLocale), TestChar);
	return IsDBCSLeadByte(TestChar);
}
HOOKFUNC BOOL WINAPI MyIsDBCSLeadByteEx(UINT CodePage, BYTE TestChar)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d) called.\n", CodePage);
	if(tasflags.appLocale && (CodePage == CP_ACP || CodePage == CP_OEMCP || CodePage == CP_THREAD_ACP || tls.forceLocale))
		CodePage = LocaleToCodePage(tasflags.appLocale);
	return IsDBCSLeadByteEx(CodePage, TestChar);
}
HOOKFUNC int WINAPI MyMultiByteToWideChar(UINT CodePage, DWORD dwFlags, LPCSTR lpMultiByteStr, int cbMultiByte, LPWSTR lpWideCharStr, int cchWideChar)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d) called.\n", CodePage);
	if(tasflags.appLocale && (CodePage == CP_ACP || CodePage == CP_OEMCP || CodePage == CP_THREAD_ACP || (dwFlags & LOCALE_USE_CP_ACP) || tls.forceLocale))
		CodePage = LocaleToCodePage(tasflags.appLocale);
	return MultiByteToWideChar(CodePage, dwFlags, lpMultiByteStr, cbMultiByte, lpWideCharStr, cchWideChar);
}
HOOKFUNC int WINAPI MyWideCharToMultiByte(UINT CodePage, DWORD dwFlags, LPCWSTR lpWideCharStr, int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte, LPCSTR lpDefaultChar, LPBOOL lpUsedDefaultChar)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d) called.\n", CodePage);
	if(tasflags.appLocale && (CodePage == CP_ACP || CodePage == CP_OEMCP || CodePage == CP_THREAD_ACP || (dwFlags & LOCALE_USE_CP_ACP) || tls.forceLocale))
		CodePage = LocaleToCodePage(tasflags.appLocale);
	return WideCharToMultiByte(CodePage, dwFlags, lpWideCharStr, cchWideChar, lpMultiByteStr, cbMultiByte, lpDefaultChar, lpUsedDefaultChar);
}
HOOKFUNC int WINAPI MyGetLocaleInfoA(LCID Locale, LCTYPE LCType, LPSTR lpLCData, int cchData)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d, 0x%X) called.\n", Locale, LCType);
	if(tasflags.appLocale && (Locale == LOCALE_SYSTEM_DEFAULT || Locale == LOCALE_USER_DEFAULT || (LCType & LOCALE_USE_CP_ACP) || tls.forceLocale))
		Locale = tasflags.appLocale;
	int rv = GetLocaleInfoA(Locale, LCType, lpLCData, cchData);
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d, 0x%X) returned \"%s\".\n", Locale, LCType, lpLCData);
	return rv;
}
HOOKFUNC int WINAPI MyGetLocaleInfoW(LCID Locale, LCTYPE LCType, LPWSTR lpLCData, int cchData)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d, 0x%X) called.\n", Locale, LCType);
	if(tasflags.appLocale && (Locale == LOCALE_SYSTEM_DEFAULT || Locale == LOCALE_USER_DEFAULT || tls.forceLocale))
		Locale = tasflags.appLocale;
	int rv = GetLocaleInfoW(Locale, LCType, lpLCData, cchData);
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d, 0x%X) returned \"%S\".\n", Locale, LCType, lpLCData);
	return rv;
}
HOOKFUNC int WINAPI MyLCMapStringA(LCID Locale, DWORD dwMapFlags, LPCSTR lpSrcStr, int cchSrc, LPSTR lpDestStr, int cchDest)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d) called.\n", Locale);
	if(tasflags.appLocale && (Locale == LOCALE_SYSTEM_DEFAULT || Locale == LOCALE_USER_DEFAULT || (dwMapFlags & LOCALE_USE_CP_ACP) || tls.forceLocale))
		Locale = tasflags.appLocale;
	return LCMapStringA(Locale, dwMapFlags, lpSrcStr, cchSrc, lpDestStr, cchDest);
}
HOOKFUNC int WINAPI MyLCMapStringW(LCID Locale, DWORD dwMapFlags, LPCWSTR lpSrcStr, int cchSrc, LPWSTR lpDestStr, int cchDest)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d) called.\n", Locale);
	if(tasflags.appLocale && (Locale == LOCALE_SYSTEM_DEFAULT || Locale == LOCALE_USER_DEFAULT || tls.forceLocale))
		Locale = tasflags.appLocale;
	return LCMapStringW(Locale, dwMapFlags, lpSrcStr, cchSrc, lpDestStr, cchDest);
}
HOOKFUNC UINT WINAPI MyGetKBCodePage()
{
	debuglog(LCF_REGISTRY, __FUNCTION__ " called.\n");
	if(tasflags.appLocale)
		return tasflags.appLocale;
	return GetKBCodePage();
}
HOOKFUNC LCID WINAPI MyConvertDefaultLocale(LCID Locale)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ " called.\n");
	if(tasflags.appLocale && (Locale == LOCALE_SYSTEM_DEFAULT || Locale == LOCALE_USER_DEFAULT || tls.forceLocale))
		return tasflags.appLocale;
	return ConvertDefaultLocale(Locale);
}
HOOKFUNC LCID WINAPI MyGetThreadLocale()
{
	debuglog(LCF_REGISTRY, __FUNCTION__ " called.\n");
	if(tasflags.appLocale)
		return tasflags.appLocale;
	return GetThreadLocale();
}
HOOKFUNC LCID WINAPI MyGetSystemDefaultLCID()
{
	debuglog(LCF_REGISTRY, __FUNCTION__ " called.\n");
	if(tasflags.appLocale)
		return tasflags.appLocale;
	return GetSystemDefaultLCID();
}
HOOKFUNC LCID WINAPI MyGetUserDefaultLCID()
{
	debuglog(LCF_REGISTRY, __FUNCTION__ " called.\n");
	if(tasflags.appLocale)
		return tasflags.appLocale;
	return GetUserDefaultLCID();
}
HOOKFUNC LANGID WINAPI MyGetSystemDefaultUILanguage()
{
	debuglog(LCF_REGISTRY, __FUNCTION__ " called.\n");
	if(tasflags.appLocale)
		return tasflags.appLocale;
	return GetSystemDefaultUILanguage();
}

HOOKFUNC LANGID WINAPI MyGetUserDefaultUILanguage()
{
	debuglog(LCF_REGISTRY, __FUNCTION__ " called.\n");
	if(tasflags.appLocale)
		return tasflags.appLocale;
	return GetUserDefaultUILanguage();
}

HOOKFUNC LANGID WINAPI MyGetSystemDefaultLangID()
{
	debuglog(LCF_REGISTRY, __FUNCTION__ " called.\n");
	if(tasflags.appLocale)
		return tasflags.appLocale;
	return GetSystemDefaultLangID();
}

HOOKFUNC LANGID WINAPI MyGetUserDefaultLangID()
{
	debuglog(LCF_REGISTRY, __FUNCTION__ " called.\n");
	if(tasflags.appLocale)
		return tasflags.appLocale;
	return GetUserDefaultLangID();
}

extern char keyboardLayoutName [KL_NAMELENGTH*2];
extern HKL g_hklOverride;

HOOKFUNC BOOL WINAPI MyGetKeyboardLayoutNameA(LPSTR pwszKLID)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ " called.\n");
	if(tasflags.appLocale)
	{
		for(int i = 0; i < 8; i++)
			pwszKLID[i] = ((tasflags.appLocale >> ((7-i) << 2)) & 0xF) + '0';
		pwszKLID[8] = 0;
		return TRUE;
	}
	if(*keyboardLayoutName)
	{
		for(int i = 0; i < 8; i++)
			pwszKLID[i] = keyboardLayoutName[i];
		pwszKLID[8] = 0;
		return TRUE;
	}
	return GetKeyboardLayoutNameA(pwszKLID);
}
HOOKFUNC BOOL WINAPI MyGetKeyboardLayoutNameW(LPWSTR pwszKLID)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ " called.\n");
	if(tasflags.appLocale)
	{
		for(int i = 0; i < 8; i++)
			pwszKLID[i] = ((tasflags.appLocale >> ((7-i) << 2)) & 0xF) + '0';
		pwszKLID[8] = 0;
		return TRUE;
	}
	if(*keyboardLayoutName)
	{
		for(int i = 0; i < 8; i++)
			pwszKLID[i] = keyboardLayoutName[i];
		pwszKLID[8] = 0;
		return TRUE;
	}
	return GetKeyboardLayoutNameW(pwszKLID);
}
HOOKFUNC HKL WINAPI MyGetKeyboardLayout(DWORD idThread)
{
	debuglog(LCF_REGISTRY|LCF_FREQUENT, __FUNCTION__ "(0x%X) called.\n", idThread);
	if(g_hklOverride)
		return g_hklOverride;
	HKL rv = GetKeyboardLayout(idThread);
	return rv;
}

HOOKFUNC BOOL WINAPI MyTranslateCharsetInfo(DWORD FAR *lpSrc, LPCHARSETINFO lpCs, DWORD dwFlags)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ " called.\n");
	ThreadLocalStuff& curtls = tls;
	curtls.forceLocale++;
	if(tasflags.appLocale)
	{
		if(dwFlags == /*TCI_SRCLOCALE*/0x1000)
			lpSrc = (DWORD*)tasflags.appLocale;
		if(dwFlags == TCI_SRCCODEPAGE)
			lpSrc = (DWORD*)LocaleToCodePage(tasflags.appLocale);
		if(dwFlags == TCI_SRCCHARSET && (DWORD)lpSrc != SYMBOL_CHARSET)
			lpSrc = (DWORD*)LocaleToCharset(tasflags.appLocale);
	}
	BOOL rv = TranslateCharsetInfo(lpSrc, lpCs, dwFlags);
	curtls.forceLocale--;
	return rv;
}
HOOKFUNC BOOL WINAPI MyTextOutA(HDC hdc, int x, int y, LPCSTR lpString, int c)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ " called.\n");
	BOOL rv;
	ThreadLocalStuff& curtls = tls;
	curtls.forceLocale++;
	//if(tasflags.appLocale) // seems unnecessary, since TextOut calls getters I've hooked
	//{
	//	str_to_wstr(wstr, lpString, LocaleToCodePage(tasflags.appLocale));
	//	rv = TextOutW(hdc, x, y, wstr, lenwstr-1);
	//}
	//else
	{
		rv = TextOutA(hdc, x, y, lpString, c);
	}
	curtls.forceLocale--;
	return rv;
}
HOOKFUNC BOOL WINAPI MyTextOutW(HDC hdc, int x, int y, LPCWSTR lpString, int c)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ " called.\n");
	ThreadLocalStuff& curtls = tls;
	curtls.forceLocale++;
	BOOL rv = TextOutW(hdc, x, y, lpString, c);
	curtls.forceLocale--;
	return rv;
}
HOOKFUNC BOOL WINAPI MyExtTextOutA(HDC hdc, int x, int y, UINT options, CONST RECT * lprect, LPCSTR lpString, UINT c, CONST INT * lpDx)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ " called.\n");
	ThreadLocalStuff& curtls = tls;
	curtls.forceLocale++;
	BOOL rv = ExtTextOutA(hdc, x, y, options, lprect, lpString, c, lpDx);
	curtls.forceLocale--;
	return rv;
}
HOOKFUNC BOOL WINAPI MyExtTextOutW(HDC hdc, int x, int y, UINT options, CONST RECT * lprect, LPCWSTR lpString, UINT c, CONST INT * lpDx)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ " called.\n");
	ThreadLocalStuff& curtls = tls;
	curtls.forceLocale++;
	BOOL rv = ExtTextOutW(hdc, x, y, options, lprect, lpString, c, lpDx);
	curtls.forceLocale--;
	return rv;
}
HOOKFUNC BOOL WINAPI MyPolyTextOutA(HDC hdc, CONST POLYTEXTA * ppt, int nstrings)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ " called.\n");
	ThreadLocalStuff& curtls = tls;
	curtls.forceLocale++;
	BOOL rv = PolyTextOutA(hdc, ppt, nstrings);
	curtls.forceLocale--;
	return rv;
}
HOOKFUNC BOOL WINAPI MyPolyTextOutW(HDC hdc, CONST POLYTEXTW * ppt, int nstrings)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ " called.\n");
	ThreadLocalStuff& curtls = tls;
	curtls.forceLocale++;
	BOOL rv = PolyTextOutW(hdc, ppt, nstrings);
	curtls.forceLocale--;
	return rv;
}


HOOKFUNC int WINAPI MyCompareStringA(LCID Locale, DWORD dwCmpFlags, LPCSTR lpString1, int cchCount1, LPCSTR lpString2, int cchCount2)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d) called.\n", Locale);
	if(tasflags.appLocale && (Locale == LOCALE_SYSTEM_DEFAULT || Locale == LOCALE_USER_DEFAULT || (dwCmpFlags & LOCALE_USE_CP_ACP) || tls.forceLocale))
		Locale = tasflags.appLocale;
	return CompareStringA(Locale, dwCmpFlags, lpString1, cchCount1, lpString2, cchCount2);
}
HOOKFUNC int WINAPI MyCompareStringW(LCID Locale, DWORD dwCmpFlags, LPCWSTR lpString1, int cchCount1, LPCWSTR lpString2, int cchCount2)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d) called.\n", Locale);
	if(tasflags.appLocale && (Locale == LOCALE_SYSTEM_DEFAULT || Locale == LOCALE_USER_DEFAULT || tls.forceLocale))
		Locale = tasflags.appLocale;
	return CompareStringW(Locale, dwCmpFlags, lpString1, cchCount1, lpString2, cchCount2);
}
HOOKFUNC int WINAPI MyGetCalendarInfoA(LCID Locale, CALID Calendar, CALTYPE CalType, LPSTR lpCalData, int cchData, LPDWORD lpValue)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d) called.\n", Locale);
	if(tasflags.appLocale && (Locale == LOCALE_SYSTEM_DEFAULT || Locale == LOCALE_USER_DEFAULT || tls.forceLocale))
		Locale = tasflags.appLocale;
	return GetCalendarInfoA(Locale, Calendar, CalType, lpCalData, cchData, lpValue);
} 
HOOKFUNC int WINAPI MyGetCalendarInfoW(LCID Locale, CALID Calendar, CALTYPE CalType, LPWSTR lpCalData, int cchData, LPDWORD lpValue)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d) called.\n", Locale);
	if(tasflags.appLocale && (Locale == LOCALE_SYSTEM_DEFAULT || Locale == LOCALE_USER_DEFAULT || tls.forceLocale))
		Locale = tasflags.appLocale;
	return GetCalendarInfoW(Locale, Calendar, CalType, lpCalData, cchData, lpValue);
}
HOOKFUNC int WINAPI MyGetTimeFormatA(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME *lpTime, LPCSTR lpFormat, LPSTR lpTimeStr, int cchTime)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d) called.\n", Locale);
	if(tasflags.appLocale && (Locale == LOCALE_SYSTEM_DEFAULT || Locale == LOCALE_USER_DEFAULT || tls.forceLocale))
		Locale = tasflags.appLocale;
	return GetTimeFormatA(Locale, dwFlags, lpTime, lpFormat, lpTimeStr, cchTime);
}
HOOKFUNC int WINAPI MyGetTimeFormatW(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME *lpTime, LPCWSTR lpFormat, LPWSTR lpTimeStr, int cchTime)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d) called.\n", Locale);
	if(tasflags.appLocale && (Locale == LOCALE_SYSTEM_DEFAULT || Locale == LOCALE_USER_DEFAULT || tls.forceLocale))
		Locale = tasflags.appLocale;
	return GetTimeFormatW(Locale, dwFlags, lpTime, lpFormat, lpTimeStr, cchTime);
}
HOOKFUNC int WINAPI MyGetDateFormatA(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME *lpDate, LPCSTR lpFormat, LPSTR lpDateStr, int cchDate)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d) called.\n", Locale);
	if(tasflags.appLocale && (Locale == LOCALE_SYSTEM_DEFAULT || Locale == LOCALE_USER_DEFAULT || tls.forceLocale))
		Locale = tasflags.appLocale;
	return GetDateFormatA(Locale, dwFlags, lpDate, lpFormat, lpDateStr, cchDate);
}
HOOKFUNC int WINAPI MyGetDateFormatW(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME *lpDate, LPCWSTR lpFormat, LPWSTR lpDateStr, int cchDate)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d) called.\n", Locale);
	if(tasflags.appLocale && (Locale == LOCALE_SYSTEM_DEFAULT || Locale == LOCALE_USER_DEFAULT || tls.forceLocale))
		Locale = tasflags.appLocale;
	return GetDateFormatW(Locale, dwFlags, lpDate, lpFormat, lpDateStr, cchDate);
}
HOOKFUNC int WINAPI MyGetNumberFormatA(LCID Locale, DWORD dwFlags, LPCSTR lpValue, CONST NUMBERFMTA *lpFormat, LPSTR lpNumberStr, int cchNumber)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d) called.\n", Locale);
	if(tasflags.appLocale && (Locale == LOCALE_SYSTEM_DEFAULT || Locale == LOCALE_USER_DEFAULT || tls.forceLocale))
		Locale = tasflags.appLocale;
	return GetNumberFormatA(Locale, dwFlags, lpValue, lpFormat, lpNumberStr, cchNumber);
}
HOOKFUNC int WINAPI MyGetNumberFormatW(LCID Locale, DWORD dwFlags, LPCWSTR lpValue, CONST NUMBERFMTW *lpFormat, LPWSTR lpNumberStr, int cchNumber)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d) called.\n", Locale);
	if(tasflags.appLocale && (Locale == LOCALE_SYSTEM_DEFAULT || Locale == LOCALE_USER_DEFAULT || tls.forceLocale))
		Locale = tasflags.appLocale;
	return GetNumberFormatW(Locale, dwFlags, lpValue, lpFormat, lpNumberStr, cchNumber);
}
HOOKFUNC int WINAPI MyGetCurrencyFormatA(LCID Locale, DWORD dwFlags, LPCSTR lpValue, CONST CURRENCYFMTA *lpFormat, LPSTR lpCurrencyStr, int cchCurrency)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d) called.\n", Locale);
	if(tasflags.appLocale && (Locale == LOCALE_SYSTEM_DEFAULT || Locale == LOCALE_USER_DEFAULT || tls.forceLocale))
		Locale = tasflags.appLocale;
	return GetCurrencyFormatA(Locale, dwFlags, lpValue, lpFormat, lpCurrencyStr, cchCurrency);
}
HOOKFUNC int WINAPI MyGetCurrencyFormatW(LCID Locale, DWORD dwFlags, LPCWSTR lpValue, CONST CURRENCYFMTW *lpFormat, LPWSTR lpCurrencyStr, int cchCurrency)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d) called.\n", Locale);
	if(tasflags.appLocale && (Locale == LOCALE_SYSTEM_DEFAULT || Locale == LOCALE_USER_DEFAULT || tls.forceLocale))
		Locale = tasflags.appLocale;
	return GetCurrencyFormatW(Locale, dwFlags, lpValue, lpFormat, lpCurrencyStr, cchCurrency);
}
HOOKFUNC DWORD WINAPI MyGetTimeZoneInformation(LPTIME_ZONE_INFORMATION lpTimeZoneInformation)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ " called.\n");
	return GetTimeZoneInformation(lpTimeZoneInformation);
}

HOOKFUNC BOOL WINAPI MySystemParametersInfoA(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d) called.\n", uiAction);
	BOOL rv = SystemParametersInfoA(uiAction, uiParam, pvParam, fWinIni);
	if(rv && tasflags.appLocale && (uiAction == SPI_GETDEFAULTINPUTLANG))
	{
		LONG& hkl = (LONG&)*(LONG*)pvParam;
		hkl = (hkl & ~0xFFFF) | (tasflags.appLocale & 0xFFFF);
	}
	return rv;
}
HOOKFUNC BOOL WINAPI MySystemParametersInfoW(UINT uiAction, UINT uiParam, PVOID pvParam, UINT fWinIni)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d) called.\n", uiAction);
	BOOL rv = SystemParametersInfoW(uiAction, uiParam, pvParam, fWinIni);
	if(rv && tasflags.appLocale && (uiAction == SPI_GETDEFAULTINPUTLANG))
	{
		LONG& hkl = (LONG&)*(LONG*)pvParam;
		hkl = (hkl & ~0xFFFF) | (tasflags.appLocale & 0xFFFF);
	}
	return rv;
}

//HOOKFUNC int WINAPI MyGetTextFaceA(HDC hdc, int c, LPSTR lpName)
//{
//	ThreadLocalStuff& curtls = tls;
//	curtls.forceLocale++;
//	int rv = GetTextFaceA(hdc, c, lpName);
//	curtls.forceLocale--;
//	debugprintf(__FUNCTION__" returned %S\n", lpName);
//	return rv;
//}
//HOOKFUNC int WINAPI MyGetTextFaceW(HDC hdc, int c, LPWSTR lpName)
//{
//	ThreadLocalStuff& curtls = tls;
//	curtls.forceLocale++;
//	int rv = GetTextFaceW(hdc, c, lpName);
//	curtls.forceLocale--;
//	debugprintf(__FUNCTION__" returned %S\n", lpName);
//	return rv;
//}


HOOKFUNC BOOL WINAPI MyGetStringTypeA(LCID Locale, DWORD dwInfoType, LPCSTR lpSrcStr, int cchSrc, LPWORD lpCharType)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d) called.\n", Locale);
	if(tasflags.appLocale && (Locale == LOCALE_SYSTEM_DEFAULT || Locale == LOCALE_USER_DEFAULT || tls.forceLocale))
		Locale = tasflags.appLocale;
	return GetStringTypeA(Locale, dwInfoType, lpSrcStr, cchSrc, lpCharType);
}
HOOKFUNC BOOL WINAPI MyGetStringTypeW(DWORD dwInfoType, LPCWSTR lpSrcStr, int cchSrc, LPWORD lpCharType)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ " called.\n");
	ThreadLocalStuff& curtls = tls;
	tls.forceLocale++;
	BOOL rv = GetStringTypeW(dwInfoType, lpSrcStr, cchSrc, lpCharType);
	tls.forceLocale--;
	return rv;
}
HOOKFUNC BOOL WINAPI MyGetStringTypeExA(LCID Locale, DWORD dwInfoType, LPCSTR lpSrcStr, int cchSrc, LPWORD lpCharType)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d) called.\n", Locale);
	if(tasflags.appLocale && (Locale == LOCALE_SYSTEM_DEFAULT || Locale == LOCALE_USER_DEFAULT || tls.forceLocale))
		Locale = tasflags.appLocale;
	return GetStringTypeExA(Locale, dwInfoType, lpSrcStr, cchSrc, lpCharType);
}
HOOKFUNC BOOL WINAPI MyGetStringTypeExW(LCID Locale, DWORD dwInfoType, LPCWSTR lpSrcStr, int cchSrc, LPWORD lpCharType)
{
	debuglog(LCF_REGISTRY, __FUNCTION__ "(%d) called.\n", Locale);
	if(tasflags.appLocale && (Locale == LOCALE_SYSTEM_DEFAULT || Locale == LOCALE_USER_DEFAULT || tls.forceLocale))
		Locale = tasflags.appLocale;
	return GetStringTypeExW(Locale, dwInfoType, lpSrcStr, cchSrc, lpCharType);
}

HOOKFUNC VOID WINAPI MySetFileApisToOEM()
{
	if(tasflags.appLocale)
		return;
	SetFileApisToOEM();
}
HOOKFUNC VOID WINAPI MySetFileApisToANSI()
{
	if(tasflags.appLocale)
		return;
	SetFileApisToANSI();
}









void ApplyRegistryIntercepts()
{
	static const InterceptDescriptor intercepts [] = 
	{
		//MAKE_INTERCEPT(1, ADVAPI32, RegQueryValueExA),
		//MAKE_INTERCEPT(1, ADVAPI32, RegOpenKeyA),
		//MAKE_INTERCEPT(1, ADVAPI32, RegOpenKeyExA),
		//MAKE_INTERCEPT(1, ADVAPI32, RegEnumKeyA),
		//MAKE_INTERCEPT(-1, NTDLL, NtQueryKey),

		//MAKE_INTERCEPT(1, ADVAPI32, RegOpenKeyW),
		//MAKE_INTERCEPT(1, ADVAPI32, RegOpenKeyExW),
		//MAKE_INTERCEPT(1, ADVAPI32, RegEnumKeyW),
		//MAKE_INTERCEPT(1, ADVAPI32, RegQueryValueExW),
		//TODO: should be possible to replace those with: NtCreateKey, NtOpenKey, NtOpenKeyEx, NtQueryValueKey ... refer to article h ttp://www.codeproject.com/KB/system/NtRegistry.aspx

		MAKE_INTERCEPT(1, USER32, GetSystemMetrics),

		MAKE_INTERCEPT(1, USER32, GetKeyboardLayoutNameA),
		MAKE_INTERCEPT(1, USER32, GetKeyboardLayoutNameW),
		MAKE_INTERCEPT(1, USER32, GetKeyboardLayout),
	};
	ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));

	static InterceptDescriptor localeIntercepts [] = 
	{
		MAKE_INTERCEPT(1, KERNEL32, GetACP),
		MAKE_INTERCEPT(1, KERNEL32, GetOEMCP),
		MAKE_INTERCEPT(1, KERNEL32, GetCPInfo),
		MAKE_INTERCEPT(1, KERNEL32, GetCPInfoExA),
		MAKE_INTERCEPT(1, KERNEL32, GetCPInfoExW),
		MAKE_INTERCEPT(1, KERNEL32, GetLocaleInfoA),
		MAKE_INTERCEPT(1, KERNEL32, GetLocaleInfoW),
		MAKE_INTERCEPT(1, KERNEL32, LCMapStringA),
		MAKE_INTERCEPT(1, KERNEL32, LCMapStringW),
		MAKE_INTERCEPT(1, KERNEL32, IsDBCSLeadByte),
		MAKE_INTERCEPT(1, KERNEL32, IsDBCSLeadByteEx),
		MAKE_INTERCEPT(1, KERNEL32, MultiByteToWideChar),
		MAKE_INTERCEPT(1, KERNEL32, WideCharToMultiByte),
		MAKE_INTERCEPT(1, USER32, GetKBCodePage),
		MAKE_INTERCEPT(1, KERNEL32, ConvertDefaultLocale),
		MAKE_INTERCEPT(1, KERNEL32, GetThreadLocale),
		MAKE_INTERCEPT(1, KERNEL32, GetSystemDefaultLCID),
		MAKE_INTERCEPT(1, KERNEL32, GetUserDefaultLCID),
		MAKE_INTERCEPT(1, KERNEL32, GetSystemDefaultUILanguage),
		MAKE_INTERCEPT(1, KERNEL32, GetUserDefaultUILanguage),
		MAKE_INTERCEPT(1, KERNEL32, GetSystemDefaultLangID),
		MAKE_INTERCEPT(1, KERNEL32, GetUserDefaultLangID),
		MAKE_INTERCEPT(1, GDI32, TranslateCharsetInfo),
		MAKE_INTERCEPT(1, GDI32, TextOutA),
		MAKE_INTERCEPT(1, GDI32, TextOutW),
		MAKE_INTERCEPT(1, GDI32, ExtTextOutA),
		MAKE_INTERCEPT(1, GDI32, ExtTextOutW),
		MAKE_INTERCEPT(1, GDI32, PolyTextOutA),
		MAKE_INTERCEPT(1, GDI32, PolyTextOutW),
		MAKE_INTERCEPT(1, KERNEL32, CompareStringA),
		MAKE_INTERCEPT(1, KERNEL32, CompareStringW),
		MAKE_INTERCEPT(1, KERNEL32, GetCalendarInfoA),
		MAKE_INTERCEPT(1, KERNEL32, GetCalendarInfoW),
		MAKE_INTERCEPT(1, KERNEL32, GetTimeFormatA),
		MAKE_INTERCEPT(1, KERNEL32, GetTimeFormatW),
		MAKE_INTERCEPT(1, KERNEL32, GetDateFormatA),
		MAKE_INTERCEPT(1, KERNEL32, GetDateFormatW),
		MAKE_INTERCEPT(1, KERNEL32, GetNumberFormatA),
		MAKE_INTERCEPT(1, KERNEL32, GetNumberFormatW),
		MAKE_INTERCEPT(1, KERNEL32, GetCurrencyFormatA),
		MAKE_INTERCEPT(1, KERNEL32, GetCurrencyFormatW),
		MAKE_INTERCEPT(1, KERNEL32, GetTimeZoneInformation),
		MAKE_INTERCEPT(1, USER32, SystemParametersInfoA),
		MAKE_INTERCEPT(1, USER32, SystemParametersInfoW),
		//MAKE_INTERCEPT(1, GDI32, GetTextFaceA),
		//MAKE_INTERCEPT(1, GDI32, GetTextFaceW),
		MAKE_INTERCEPT(1, KERNEL32, GetStringTypeA),
		MAKE_INTERCEPT(1, KERNEL32, GetStringTypeW),
		MAKE_INTERCEPT(1, KERNEL32, GetStringTypeExA),
		MAKE_INTERCEPT(1, KERNEL32, GetStringTypeExW),
		MAKE_INTERCEPT(1, KERNEL32, SetFileApisToOEM),
		MAKE_INTERCEPT(1, KERNEL32, SetFileApisToANSI),
	};
	for(int i = 0; i < ARRAYSIZE(localeIntercepts); i++)
		localeIntercepts[i].enabled = (tasflags.appLocale != 0) ? 1 : 0;
	ApplyInterceptTable(localeIntercepts, ARRAYSIZE(localeIntercepts));
}


#else
#pragma message(__FILE__": (skipped compilation)")
#endif

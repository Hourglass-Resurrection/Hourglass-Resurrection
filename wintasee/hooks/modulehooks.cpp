/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(MODULEHOOKS_INCL) && !defined(UNITY_BUILD)
#define MODULEHOOKS_INCL

#include "../global.h"
#include "../../shared/ipc.h"
#include "../../shared/winutil.h"
#include "../tls.h"
#include "../wintasee.h"
#include <map>

bool TrySoundCoCreateInstance(REFIID riid, LPVOID *ppv);

//static char currentModuleFilename [MAX_PATH+1] = {0};
//static char dlltempDir [MAX_PATH+1] = {0};
//
//__declspec(noinline) const char* GetFullDLLPath(LPCWSTR wfilename)
//{
//	char filename [MAX_PATH+1];
//	sprintf(filename, "%S", wfilename);
//
//	char path [MAX_PATH+1] = {0};
//	char* filenameonly = filename;
//	static char result [2*MAX_PATH+1] = {0};
//	// temporary, should really follow the official "Dynamic-Link Library Search Order"
//
//	if(SearchPathA(NULL, filename, NULL, MAX_PATH, path, &filenameonly))
//	{
//		sprintf(result, "%s\\%s", dlltempDir, filenameonly);
//		CopyFile(path, result, FALSE);
//	}
//	else
//	{
//		strcpy(result, filename);
//	}
//
//	//debugprintf("AAA: %s\n", result);
//
//	return result;
//}

//#include <set>
//#include <string>
//std::set<std::string> deniedList;
//
//// FIXME: list should be configurable by user from wintaser app (well, for a while now it's been disabled)
//static char dllLeaveAloneList [256][MAX_PATH+1] =
//{"uxtheme.dll", "SETUPAPI.DLL",
//"KsUser.dll", "dinput.dll", "dinput8.dll",
//"cctrans.dll", "clbcatq.dll",
//"dmsynth.dll", "dmloader.dll", "dmstyle.dll", "dmband.dll", "dmime.dll", "dmusic.dll", "midimap.dll",
//"ddraw.dll", "d3d8.dll", "d3dim.dll", "d3dim700.dll",
//
//"kernel32.dll", "user32.dll", "gdi32.dll", "winspool.dll", "comdlg32.dll", "advapi32.dll",
//"shell32.dll", "ole32.dll", "oleaut32.dll", "uuid.dll", "odbc32.dll", "odbccp32.dll",
//
////"wdmaud.drv", "msacm32.drv" // needed, but auto-allowed because of non-dll extension
//};
//// testing disabling as many DLLs as possible
//BOOL ShouldAllowDLLLoad(LPCSTR filename)
//{
//	{
//		const char* dot = strrchr(filename, '.');
//		if(dot)
//		{
//			if(stricmp(dot, ".dll") != 0)
//			{
//				return TRUE;
//			}
//		}
//		for(int i = 0; i < ARRAYSIZE(dllLeaveAloneList); i++)
//		{
//			const char* dllstr = dllLeaveAloneList[i];
//			const char* pfilename = strrchr(filename, '\\') + 1;
//			if(pfilename == (const char*)1)
//				pfilename = filename;
//			if(!stricmp(dllstr, pfilename))
//				return TRUE;
//		}
//	}
//
//	HANDLE handle = GetModuleHandleA(filename);
//	if(handle == NULL)
//	{
//		if(deniedList.find(filename) == deniedList.end())
//			debuglog(LCF_MODULE|LCF_TODO, "DENIED loading DLL: %s\n", filename);
//		deniedList.insert(filename);
//		SetLastError(ERROR_MOD_NOT_FOUND);
//		return FALSE;
//	}
//
//	return TRUE;
//}

//HOOKFUNC HMODULE WINAPI MyLoadLibraryExW(LPCWSTR lpFileName, HANDLE hFile, DWORD dwFlags)
//{
//	debugprintf(__FUNCTION__ "(%S, 0x%X, 0x%X)\n", lpFileName, hFile, dwFlags);
//cmdprintf("SHORTTRACE: 3,50");
//
//	//char name [1024];
//	//sprintf(name, "%S", lpFileName); // the internal LoadLibrary function (LdrLoadDll) overwrites lpFileName
//	//if(!ShouldAllowDLLLoad(lpFileName))
//	//{
//	//	SetLastError(ERROR_MOD_NOT_FOUND);
//	//	return 0;
//	//}
//	HMODULE rv = LoadLibraryExW(lpFileName, hFile, dwFlags);
//	//debugprintf("BBBBB: 0x%X, %S\n", rv, lpFileName);
//	//if(rv)
//	//{
//	//	char dllname [1024];
//	//	sprintf(dllname, "%S", lpFileName);
//	//	//debugprintf("CCCCC: %s\n", dllname);
//	//	RetryInterceptAPIs(dllname);
//	//}
//	return rv;
//}

DllLoadInfos dllLoadInfos = {};

static CRITICAL_SECTION s_dllLoadAndRetryInterceptCS;

// consumes data sent by AddAndSendDllInfo
void UpdateLoadedOrUnloadedDllHooks()
{
	AutoCritSect cs(&s_dllLoadAndRetryInterceptCS);

	//for(int i = 0; i < dllLoadInfos.numInfos; i++)
	//	debugprintf("dllLoadInfos[%d].dllname = %s\n", i, dllLoadInfos.infos[i].dllname);
	while(dllLoadInfos.numInfos > 0)
	{
		DllLoadInfo info = dllLoadInfos.infos[--dllLoadInfos.numInfos];
		if(info.loaded)
			RetryInterceptAPIs(info.dllname);
		//else
		//	UnInterceptUnloadingAPIs(info.dllname);
	}
}



// MyLdrUnloadDll disabled because
// there's no need to call UpdateLoadedOrUnloadedDllHooks immediately when an unload happens 
// since MyLdrLoadDll will handle it (UpdateLoadedOrUnloadedDllHooks),
// and these NTDLL functions can be sort of unstable so I leave them alone when possible.
//HOOKFUNC NTSTATUS NTAPI MyLdrUnloadDll(HANDLE ModuleAddress)
//{
//#if 1 // new method, get list of loaded dlls from debugger
//	NTSTATUS rv = LdrUnloadDll(ModuleAddress);
//	UpdateLoadedOrUnloadedDllHooks();
//	return rv;
//#else
//	// unloading DLLs takes time
//	// uh... the game tries to unload its own .exe every frame??? disabled here//not:, and extra time has been added to the load instead...
////	detTimer.AddDelay(/*10*/, FALSE, FALSE); // must both be FALSE to signal async delay add, otherwise really weird things will happen like inaccurate thread creation reports to the debugger
//
//	HANDLE ModuleHandle = 0;
//	HANDLE hProcess = GetCurrentProcess();
//
//	//static HMODULE hMods [1024];
//	//MODULEINFO info;
//	//DWORD cbNeeded;
//	//if(EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
//	//{
//	//	for(unsigned int i=0;i<(cbNeeded/sizeof(HMODULE));i++)
//	//	{
//	//		if(GetModuleInformation(hProcess, hMods[i], &info, sizeof(info)))
//	//		{
//	//			debugprintf("0x%X ==? 0x%x, modhand=0x%X\n", info.lpBaseOfDll, ModuleAddress, hMods[i]);
//	//			if(info.lpBaseOfDll == (void*)ModuleAddress)
//	//			{
//	//				ModuleHandle = hMods[i];
//	//				break;
//	//			}
//	//		}
//	//	}
//	//}
//
//	char baseName [1024];
//	DWORD baseNameLen = GetModuleBaseName(hProcess, (HMODULE)ModuleAddress, baseName, sizeof(baseName));
//
//	NTSTATUS rv = LdrUnloadDll(ModuleAddress);
//
//	if(rv >= 0)
//	{
//		if(baseNameLen)
//		{
//			char* slash = max(strrchr(baseName, '\\'), strrchr(baseName, '/'));
//			const char* dllname = slash ? slash+1 : baseName;
//			//debuglog(LCF_MODULE, __FUNCTION__ "(%s, 0x%X)\n", baseName, ModuleAddress);
//			UnInterceptUnloadingAPIs(dllname);
//		}
//	}
//	else
//	{
//		debuglog(LCF_MODULE|LCF_ERROR, "FAILED to unload DLL: %s (addr=0x%X) (rv=0x%X)\n", baseNameLen?baseName:"", ModuleAddress, rv);
//	}
//
//
//	return rv;
//#endif
//}

static BOOL ShouldLoadUserDll(LPWSTR lpFileName)
{
	if(tasflags.allowLoadInstalledDlls == tasflags.allowLoadUxtheme)
		return!! tasflags.allowLoadInstalledDlls;
	char name [1024];
	{
		int i = 0;
		for(; lpFileName[i] && i != sizeof(name)-1; i++)
			name[i] = tolower((char)lpFileName[i]);
		name[i] = 0;
	}
	char* slash = strrchr(name, '\\');
	const char* dllname = slash ? slash+1 : name;
	bool isUxTheme = !strcmp(dllname, "uxtheme.dll") || !strcmp(dllname, "themeui.dll") || !strcmp(dllname, "themeservice.dll");
	if(isUxTheme)
		return tasflags.allowLoadUxtheme;
	return tasflags.allowLoadInstalledDlls;
}

static BOOL WideStringContains(LPWSTR lpFileName, const char* match)
{
	char name [1024];
	{
		int i = 0;
		for(; lpFileName[i] && i != sizeof(name)-1; i++)
			name[i] = tolower((char)lpFileName[i]);
		name[i] = 0;
	}
	return (BOOL)strstr(name, match);
}

bool watchForCLLApiNum = false;
int cllApiNum = -1;

//void debugsplatmem(DWORD address, const char* name);

HOOKFUNC NTSTATUS NTAPI MyLdrLoadDll(PWCHAR PathToFile, ULONG Flags, PUNICODE_STRING ModuleFileName, PHANDLE ModuleHandle)
{
//debugprintf(__FUNCTION__ "(ModuleFileName=\"%S\") called.\n", ModuleFileName->Buffer);
//cmdprintf("SHORTTRACE: 3,50");

	//debugprintf(__FUNCTION__ "(ModuleFileName=\"%S\") called.\n", ModuleFileName->Buffer);
	//DWORD myEBP;
 //   __asm
 //   {
 //     mov [myEBP], ebp;
 //   }
	//debugsplatmem(myEBP, "ebp");

#if 1 // new method, get list of loaded dlls from debugger (because LdrLoadDll can load multiple dlls)
	//_asm{int 3} // to print callstack... debugprintf/cmdprintf can cause problems in this context
	ThreadLocalStuff* pCurtls = 0;
	if(tlsIsSafeToUse)
	{
		pCurtls = &tls;
		ThreadLocalStuff& curtls = *pCurtls;
		if(curtls.callingClientLoadLibrary || curtls.treatDLLLoadsAsClient)
		{
			curtls.callingClientLoadLibrary = FALSE; // see MyKiUserCallbackDispatcher
			watchForCLLApiNum = false; // if we were watching for the apiNum, it must have worked
			if(!ShouldLoadUserDll(ModuleFileName->Buffer))
			{
				//debuglog(LCF_MODULE, "DENIED loading DLL: %S\n", ModuleFileName->Buffer);
				//cmdprintf("SHORTTRACE: 3,50");
				return /*STATUS_DLL_NOT_FOUND*/0xC0000135;
			}
		}
		
		// TEST HACK
		if(WideStringContains(ModuleFileName->Buffer, "dpofeedb.dll"))
			return /*STATUS_DLL_NOT_FOUND*/0xC0000135;

		pCurtls->callerisuntrusted++;
	}
	//if(tlsIsSafeToUse)
	//{
	//	debuglog(LCF_MODULE, "Loaded DLL: %S\n", ModuleFileName->Buffer);
	//	//cmdprintf("SHORTTRACE: 3,50");
	//}

	NTSTATUS rv = LdrLoadDll(PathToFile, Flags, ModuleFileName, ModuleHandle);
	//if(rv < 0)
	//	debuglog(LCF_MODULE|LCF_ERROR, "FAILED to load DLL: %S (0x%X)\n", ModuleFileName->Buffer, rv);
	UpdateLoadedOrUnloadedDllHooks();
	if(pCurtls)
		pCurtls->callerisuntrusted--;
	return rv;
#else

	// for some reason this function is INCREDIBLY fragile.
	// the slightest bit too much processing will make games fail to load certain critical DLLs.
	// I'd like to call ShouldAllowDLLLoad to deny certain DLLs from loading, but currently can't.

	LPWSTR lpFileName = ModuleFileName->Buffer;

	// store the string first because LdrLoadDll puts garbage in the string (registry keys?)
	// this way of doing it seems to be more reliable than sprintf(name, "%S", lpFileName);
	char name [1024];
	int i = 0;
	for(; lpFileName[i] && i != sizeof(name)-1; i++)
		name[i] = (char)lpFileName[i];
	name[i] = 0;

	//debuglog(LCF_MODULE, __FUNCTION__ "(%S, 0x%X, %S, 0x%X)\n", lpFileName, Flags, PathToFile, ModuleHandle);
	//if(!ShouldAllowDLLLoad(name))
	//{
	//	//return 0xC0000135;
	//}

	NTSTATUS rv = LdrLoadDll(PathToFile, Flags, ModuleFileName, ModuleHandle);

	//static int inside = 0;
	//if(inside) if(inside != GetCurrentThreadId()) while(inside) {OutputDebugString("WTFA\n");}
	//inside = GetCurrentThreadId();

//#if defined(_MSC_VER) && _MSC_VER >= 1400 && _MSC_VER < 1500
	// terrible mystery hack! to fix some games from failing to find kernel32.dll when this file is compiled with VS2005
	//static bool already = false;
	//if(!already)
//	if(lpFileName[0] == 'K' && lpFileName[6] == '3')
//	{
//		//already = true;
//		NTSTATUS rv = LdrLoadDll(PathToFile, Flags, ModuleFileName, ModuleHandle);
////		inside = 0;
//		return rv;
//	}
//#endif


//	NTSTATUS rv = LdrLoadDll(PathToFile, Flags, ModuleFileName, ModuleHandle);

	if(rv >= 0)
	{
		int namelen = i;
		if(!(namelen < 4 || !stricmp(name+namelen-4,".exe")))
		{
			char* slash = max(strrchr(name, '\\'), strrchr(name, '/'));
			const char* dllname = slash ? slash+1 : name;
			//debuglog(LCF_MODULE, "Rehooking: %s\n", dllname);
			RetryInterceptAPIs(dllname);
// disabled because it will make AVIs captured on different machines be slightly different lengths
			//// loading DLLs takes time
			//detTimer.AddDelay(/*10*/15, FALSE, FALSE); // must both be FALSE to signal async delay add, otherwise really weird things will happen like inaccurate thread creation reports to the debugger
		}
	}
	else
	{
		//HANDLE handle = GetModuleHandleA(name);
		//if(handle)
		//{
		//	*ModuleHandle = handle;
		//	rv = 0;
		//}
		//while(rv < 0)
		{
			debuglog(LCF_MODULE|LCF_ERROR, "FAILED to load DLL: %s (0x%X)\n", name, rv);
//			debuglog(LCF_MODULE|LCF_ERROR, "%S, 0x%X, 0x%X, %S\n", ModuleFileName->Buffer, Flags, ModuleHandle, PathToFile);
//			rv = LdrLoadDll(PathToFile, Flags, ModuleFileName, ModuleHandle);
		}

	//NTSTATUS rv = LdrLoadDll(PathToFile, Flags, ModuleFileName, ModuleHandle);
	}

//	inside = 0;
	return rv;
#endif
}


//static char dllLeaveAloneList [256][MAX_PATH+1] = {};
//
//HOOKFUNC HMODULE WINAPI MyLoadLibraryExW(LPCWSTR lpFileName, HANDLE hFile, DWORD dwFlags)
//{
//	{
//		char filename [MAX_PATH+1];
//		sprintf(filename, "%S", lpFileName);
//
//		const char* dot = strrchr(filename, '.');
//		if(dot)
//		{
//			if(!stricmp(dot, ".exe"))
//			{
////				debuglog(LCF_MODULE, "REGULAR: %s\n", filename);
//				return LoadLibraryExW(lpFileName, hFile, dwFlags);
//			}
//		}
//		for(int i = 0; i < ARRAYSIZE(dllLeaveAloneList); i++)
//		{
//			const char* dllstr = dllLeaveAloneList[i];
//			const char* pfilename = strrchr(filename, '\\') + 1;
//			if(pfilename == (const char*)1)
//				pfilename = filename;
//			if(!stricmp(dllstr, pfilename))
//			{
////				debuglog(LCF_MODULE, "REGULAR: %s\n", filename);
//				return LoadLibraryExW(lpFileName, hFile, dwFlags);
//			}
//			else
//			{
////				if(*dllstr)
////				debuglog(LCF_MODULE, "NOMATCH: %s != %s\n", filename, dllstr);
//			}
//		}
//	}
//
//	const char* fullPath = GetFullDLLPath(lpFileName);
//
//	debuglog(LCF_MODULE, __FUNCTION__ "(\"%S\") -> \"%s\"\n", lpFileName, fullPath);
//
//	//HMODULE rv = LoadLibraryExW(lpFileName, hFile, dwFlags);
//
//	WCHAR wfullPath [MAX_PATH+1];
//	for(int i = 0; fullPath[i]; i++)
//		wfullPath[i] = fullPath[i];
//
//	//debugprintf("???: %S\n", wstr);
//
//	//HMODULE rv = LoadLibraryExW(lpFileName, hFile, dwFlags);
//	HMODULE rv = LoadLibraryExW(wfullPath, hFile, dwFlags);
//
//
//	return rv;
//}

//HOOKFUNC FARPROC WINAPI MyGetProcAddress(HMODULE hModule, LPCSTR lpProcName)
//{
//	debugprintf(__FUNCTION__ "(\"%s\") called.\n", lpProcName);
//	FARPROC rv = GetProcAddress(hModule, lpProcName);
//	return rv;
//}

HOOKFUNC VOID NTAPI MyKiUserCallbackDispatcher(ULONG ApiNumber, PVOID InputBuffer, ULONG InputLength)
{
	//debugprintf(__FUNCTION__ "(ApiNumber=%d) called.\n",ApiNumber);

	// maybe should instead scan the stack in MyLdrLoadDll for something we put on the stack in MyKiUserCallbackDispatcher? but I couldn't get it to work...
//	char test [8] = {0,0x42,0x42,0x42,0x42,0x42,0x42,0x42,};
//	debugprintf(test);

	if(watchForCLLApiNum)
		cllApiNum = ApiNumber;

	if(ApiNumber == cllApiNum)
		tls.callingClientLoadLibrary = TRUE;

	KiUserCallbackDispatcher(ApiNumber, InputBuffer, InputLength);
	// at least on Windows XP, code placed here won't run,
	// because KiUserCallbackDispatcher returns directly to the kernel mode code that called us.
	// so, so we have to reset tls.callingClientLoadLibrary elsewhere (in MyLdrLoadDll)
}

// TODO it's just for debugging but this is kind of wrong (chooseriid, riidToName)
inline REFIID chooseriid(REFIID riid, REFCLSID rclsid)
{
	if(riid.Data1 > 1)
		return riid;
	return (REFIID)rclsid;
}
const char* riidToName(REFIID riid)
{
	switch(riid.Data1)
	{
	case 0x56A8689F:
		return "IFilterGraph";
	case 0x56A8689C:
		return "IMemAllocator";

// not sure which of these are helpful, I'm just noting them down for now
	case 0x56A86895:
		return "IBaseFilter";
	case 0xE436EBB3:
		return "FilgraphManager";
	case 0XE436EBB8:
		return "FilgraphManagerNoThread";
	case 0xCDA42200:
		return "FilterMapper2";
	case 0xE436EBB5:
		return "AsyncReader";
	case 0x4315D437:
		return "CDeviceMoniker";
	case 0xE21BE468:
		return "RealSplitter";
	case 0x336475D0:
		return "MpegSplitter";
	case 0x0F40E1E5:
		return "ffdshowAudio";
	case 0x79376820:
		return "DirectSoundRender";
	case 0x1E651CC0:
		return "MemoryAllocator";
	case 0x060AF76C:
		return "SeekingPassThru";
	}
	
	return NULL;
}

void UpdateLoadedOrUnloadedDllHooks();

// in case either MyCoCreateInstance doesn't call MyCoCreateInstanceEx or MyCoCreateInstance is called and MyCoCreateInstanceEx failed to get hooked
HOOKFUNC HRESULT STDAPICALLTYPE MyCoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv)
{
	ThreadLocalStuff& curtls = tls;
	curtls.callerisuntrusted++;
	const char* oldName = curtls.curThreadCreateName;
	const char* newName = riidToName(chooseriid(riid,rclsid));
	debuglog(LCF_MODULE, __FUNCTION__ "(0x%X, 0x%X (%s)) called.\n", riid.Data1, rclsid.Data1, newName?newName:"?");
	if(newName)
		curtls.curThreadCreateName = newName;

	HRESULT rv = E_FAIL;

	if(TrySoundCoCreateInstance(riid, ppv))
	{
		rv = S_OK;
	}
	else
	{
		// normal case
		rv = CoCreateInstance(rclsid, pUnkOuter, dwClsContext, riid, ppv);
		if(SUCCEEDED(rv))
			HookCOMInterface(riid, ppv);
	}
	if(newName)
		curtls.curThreadCreateName = oldName;
	UpdateLoadedOrUnloadedDllHooks();
	curtls.callerisuntrusted--;
	return rv;
}


DEFINE_LOCAL_GUID(CLSID_FilterGraphManager, 0xe436ebb3,0x524f,0x11ce,0x9f,0x53,0x00,0x20,0xaf,0x0b,0xa7,0x70);
DEFINE_LOCAL_GUID(CLSID_FilterGraphNoThread,0xe436ebb8,0x524f,0x11ce,0x9f,0x53,0x00,0x20,0xaf,0x0b,0xa7,0x70);

static void PreCoGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID FAR* ppv, const char* callerName, const char* oldName)
{
	const char* newName = riidToName(chooseriid(riid,rclsid));
	debuglog(LCF_MODULE, __FUNCTION__ "(0x%X, 0x%X (%s)) called by %s.\n", riid.Data1, rclsid.Data1, newName?newName:"?", callerName);
	if(rclsid.Data1 == CLSID_FilterGraphManager.Data1 /*&& tasflags.threadMode < 2*/)
		((IID&)rclsid).Data1 = CLSID_FilterGraphNoThread.Data1; // here's hoping this helps
	if(!oldName && !newName)
		newName = "DirectShow"; // TODO
	ThreadLocalStuff& curtls = tls;
	if(newName)
		curtls.curThreadCreateName = newName;
	curtls.callerisuntrusted++;
}
static void PostCoGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID FAR* ppv, HRESULT hr, const char* oldName)
{
	ThreadLocalStuff& curtls = tls;
	curtls.callerisuntrusted--;
	if(SUCCEEDED(hr))
		HookCOMInterface(riid, ppv);
	//if(newName)
		curtls.curThreadCreateName = oldName;
	UpdateLoadedOrUnloadedDllHooks();
}

HOOKFUNC HRESULT STDAPICALLTYPE MyCoGetClassObject(REFCLSID rclsid, DWORD dwClsContext, LPVOID pvReserved, REFIID riid, LPVOID FAR* ppv)
{
	const char* oldName = tls.curThreadCreateName;
	PreCoGetClassObject(rclsid,riid,ppv, __FUNCTION__, oldName);
	HRESULT rv = CoGetClassObject(rclsid, dwClsContext, pvReserved, riid, ppv);
	PostCoGetClassObject(rclsid,riid,ppv, rv, oldName);
	return rv;
}

// in case either MyCoCreateInstanceEx is directly instead of from MyCoCreateInstance, or MyCoCreateInstanceEx is called from MyCoCreateInstance but MyCoCreateInstance failed to get hooked
HOOKFUNC HRESULT STDAPICALLTYPE MyCoCreateInstanceEx(REFCLSID Clsid, LPUNKNOWN punkOuter, DWORD dwClsCtx, struct _COSERVERINFO* pServerInfo, DWORD dwCount, struct tagMULTI_QI* pResults)
{
	debuglog(LCF_MODULE, __FUNCTION__ "(clsid=0x%X, dwCount=%d) called.\n", Clsid.Data1, dwCount);

	// check for creating custom objects that skip COM
//	DEFINE_LOCAL_GUID(IID_IUnknown,0x00000000,0x0000,0x0000,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46);
	for(DWORD i = 0; i < dwCount; i++)
	{
		if(TrySoundCoCreateInstance(*pResults[i].pIID, (LPVOID*)&pResults[i].pItf))
		{
			pResults[i].hr = S_OK;
			HRESULT rv = S_OK;
			bool anyok = false;
			for(DWORD j = 0; j < dwCount; j++)
			{
				if(i != j)
				{
					pResults[j].hr = pResults[i].pItf->QueryInterface(*pResults[j].pIID, (LPVOID*)pResults[j].pItf);
					if(FAILED(pResults[j].hr))
						rv = pResults[j].hr;
					else
						anyok = true;
				}
			}
			if(anyok && FAILED(rv))
				rv = CO_S_NOTALLINTERFACES;
			return rv;
		}
	}

	// regular creation
	HRESULT rv = CoCreateInstanceEx(Clsid, punkOuter, dwClsCtx, pServerInfo, dwCount, pResults);
	for(DWORD i = 0; i < dwCount; i++)
		if(SUCCEEDED(pResults[i].hr))
			HookCOMInterface(*pResults[i].pIID, (LPVOID*)&pResults[i].pItf);
	return rv;
}

//static HRESULT STDAPICALLTYPE MyDllGetClassObject_Impl(TypeOfDllGetClassObject DllGetClassObject, const char* dllname, REFCLSID rclsid, REFIID riid, LPVOID *ppv)
//{
//	debuglog(LCF_MODULE, __FUNCTION__ " called by %s.\n", dllname);
//	const char* oldName = tls.curThreadCreateName;
//	PreCoGetClassObject(rclsid,riid,ppv, dllname, oldName);
//	HRESULT rv = DllGetClassObject(rclsid, riid, ppv);
//	PostCoGetClassObject(rclsid,riid,ppv, rv, oldName);
//	return rv;
//}
//#define X(y) HOOKFUNC HRESULT STDAPICALLTYPE My##y##DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv) { return MyDllGetClassObject_Impl(Tramp##y##DllGetClassObject, ArrayNameDllGetClassObject[y], rclsid, riid, ppv); }
//    CodeGen_X_List
//#undef X
//TypeOfDllGetClassObject ArrayMyDllGetClassObject[] = 
//{
//#define X(y) My##y##DllGetClassObject,
//    CodeGen_X_List
//#undef X
//	NULL
//};

#define IMPLEMENT_MyDllGetClassObject(suffix) \
TRAMPFUNC HRESULT STDAPICALLTYPE TrampDllGetClassObject_##suffix(REFCLSID rclsid, REFIID riid, LPVOID *ppv) TRAMPOLINE_DEF \
static HRESULT STDAPICALLTYPE MyDllGetClassObject_##suffix(REFCLSID rclsid, REFIID riid, LPVOID *ppv) \
{ \
	const char* oldName = tls.curThreadCreateName; \
	PreCoGetClassObject(rclsid,riid,ppv, #suffix, oldName); \
	HRESULT rv = TrampDllGetClassObject_##suffix(rclsid, riid, ppv); \
	PostCoGetClassObject(rclsid,riid,ppv, rv, oldName); \
	return rv; \
}
IMPLEMENT_MyDllGetClassObject(quartz)
//IMPLEMENT_MyDllGetClassObject(ffdshow) // apparently not needed
//IMPLEMENT_MyDllGetClassObject(fmodex)
//IMPLEMENT_MyDllGetClassObject(bass)


HOOKFUNC HRESULT STDMETHODCALLTYPE MyIUnknown_QueryInterface_Proxy(IUnknown __RPC_FAR * This,REFIID riid,void __RPC_FAR *__RPC_FAR *ppvObject)
{
	debuglog(LCF_MODULE|LCF_UNTESTED, __FUNCTION__ "(0x%X) called.\n", riid.Data1);
	ThreadLocalStuff& curtls = tls;
	curtls.callerisuntrusted++;
	const char* oldName = curtls.curThreadCreateName;
	const char* newName = riidToName(riid);
	if(!oldName && !newName)
		newName = "RPC";
	if(newName)
		curtls.curThreadCreateName = newName;
	HRESULT rv = IUnknown_QueryInterface_Proxy(This, riid, ppvObject);
	if(SUCCEEDED(rv))
		HookCOMInterface(riid, ppvObject);
	if(newName)
		curtls.curThreadCreateName = oldName;
	curtls.callerisuntrusted--;
	return rv;
}

struct AutoUntrust
{
	ThreadLocalStuff* pCurtls;
	AutoUntrust()
	{
		pCurtls = ThreadLocalStuff::GetIfAllocated();
		if(pCurtls)
			pCurtls->callerisuntrusted++;
	}
	~AutoUntrust()
	{
		if(pCurtls)
			pCurtls->callerisuntrusted--;
	}
};

HOOKFUNC PVOID NTAPI MyRtlAllocateHeap(PVOID HeapHandle, ULONG Flags, SIZE_T Size)
{
	AutoUntrust au;
	return RtlAllocateHeap(HeapHandle, Flags, Size);
}
HOOKFUNC PVOID NTAPI MyRtlCreateHeap(ULONG Flags, PVOID HeapBase, SIZE_T ReserveSize, SIZE_T CommitSize, PVOID Lock, struct RTL_HEAP_PARAMETERS* Parameters)
{
	AutoUntrust au;
	return RtlCreateHeap(Flags, HeapBase, ReserveSize, CommitSize, Lock, Parameters);
}
HOOKFUNC PVOID RPC_ENTRY MyNdrAllocate(PMIDL_STUB_MESSAGE pStubMsg, size_t Len)
{
	AutoUntrust au;
	return NdrAllocate(pStubMsg, Len);
}
HOOKFUNC void RPC_ENTRY MyNdrClientInitializeNew(PRPC_MESSAGE pRpcMsg, PMIDL_STUB_MESSAGE pStubMsg, PMIDL_STUB_DESC pStubDescriptor, unsigned int ProcNum)
{
	AutoUntrust au;
	return NdrClientInitializeNew(pRpcMsg, pStubMsg, pStubDescriptor, ProcNum);
}
HOOKFUNC void RPC_ENTRY MyNdrClientInitialize(PRPC_MESSAGE pRpcMsg, PMIDL_STUB_MESSAGE pStubMsg, PMIDL_STUB_DESC pStubDescriptor, unsigned int ProcNum)
{
	AutoUntrust au;
	return NdrClientInitialize(pRpcMsg, pStubMsg, pStubDescriptor, ProcNum);
}





HOOKFUNC BOOL WINAPI MyCreateProcessA(
	LPCSTR lpApplicationName,
	LPSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCSTR lpCurrentDirectory,
	LPSTARTUPINFOA lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
)
{
	debuglog(LCF_PROCESS|LCF_TODO, __FUNCTION__ " called: %s\n", lpCommandLine);
	tls.isFrameThread = FALSE;
	BOOL rv = CreateProcessA(
		lpApplicationName,
		lpCommandLine,
		lpProcessAttributes,
		lpThreadAttributes,
		bInheritHandles,
		dwCreationFlags,
		lpEnvironment,
		lpCurrentDirectory,
		lpStartupInfo,
		lpProcessInformation
	);
	return rv;
}

HOOKFUNC BOOL WINAPI MyCreateProcessW(
	LPCWSTR lpApplicationName,
	LPWSTR lpCommandLine,
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCWSTR lpCurrentDirectory,
	LPSTARTUPINFOW lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
)
{
	debuglog(LCF_PROCESS|LCF_TODO, __FUNCTION__ " called: %S\n", lpCommandLine);
	tls.isFrameThread = FALSE;
	BOOL rv = CreateProcessW(
		lpApplicationName,
		lpCommandLine,
		lpProcessAttributes,
		lpThreadAttributes,
		bInheritHandles,
		dwCreationFlags,
		lpEnvironment,
		lpCurrentDirectory,
		lpStartupInfo,
		lpProcessInformation
	);
	return rv;
}

HOOKFUNC VOID WINAPI MyExitProcess(DWORD dwExitCode)
{
	debuglog(LCF_PROCESS, __FUNCTION__ " called.\n");
	_asm{int 3}
	while(true) { Sleep(10); }
}

//HOOKFUNC NTSTATUS NTAPI MyNtQueryInformationProcess(HANDLE ProcessHandle, /*PROCESSINFOCLASS*/DWORD ProcessInformationClass, PVOID ProcessInformation, ULONG ProcessInformationLength, PULONG ReturnLength)
//{
//	//return NtQueryInformationProcess(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);
//	//debugprintf("ProcessInformationClass = 0x%X\n", ProcessInformationClass);
//	NTSTATUS rv = NtQueryInformationProcess(ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength, ReturnLength);
//	if(ProcessInformationClass == /*DebugPort*/7) *(DWORD*)ProcessInformation = 0;
//	return rv;
//}

//HOOKFUNC SC_HANDLE APIENTRY MyOpenServiceA(SC_HANDLE hSCManager, LPCSTR lpServiceName, DWORD dwDesiredAccess)
//{
//	debuglog(LCF_PROCESS|LCF_UNTESTED, __FUNCTION__ " called (and suppressed).\n");
//	// NYI?
//	SetLastError(ERROR_SERVICE_DOES_NOT_EXIST);
//	return 0;
//	//SC_HANDLE rv = OpenServiceA(hSCManager, lpServiceName, dwDesiredAccess);
//	//debugprintf("returned (%s -> 0x%X).\n", lpServiceName, hSCManager);
//	//return rv;
//}
//HOOKFUNC SC_HANDLE APIENTRY MyOpenServiceW(SC_HANDLE hSCManager, LPCWSTR lpServiceName, DWORD dwDesiredAccess)
//{
//	debuglog(LCF_PROCESS|LCF_UNTESTED, __FUNCTION__ " called (and suppressed).\n");
//	// NYI?
//	SetLastError(ERROR_SERVICE_DOES_NOT_EXIST);
//	return 0;
//	//SC_HANDLE rv = OpenServiceW(hSCManager, lpServiceName, dwDesiredAccess);
//	//debugprintf("returned (%S -> 0x%X).\n", lpServiceName, hSCManager);
//	//return rv;
//}

struct _tiddata {
    unsigned long   _tid;       /* thread ID */
    uintptr_t _thandle;         /* thread handle */
    int     _terrno;            /* errno value */
    unsigned long   _tdoserrno; /* _doserrno value */
    unsigned int    _fpds;      /* Floating Point data segment */
    unsigned long   _holdrand;  /* rand() seed value */
	//There's more than this in a full _tiddata struct, but this is probably everything we're interested in
};

typedef struct _tiddata * _ptiddata;
BOOL FlsRecursing = FALSE;
std::map<DWORD,DWORD *> fseeds;
HOOKFUNC BOOL WINAPI MyFlsSetValue(DWORD dwFlsIndex, LPVOID lpFlsData) {
	BOOL rv = FlsSetValue(dwFlsIndex,lpFlsData);
	if ((!FlsRecursing) && (lpFlsData != NULL)) {
		FlsRecursing = TRUE;
		if (fseeds.find(dwFlsIndex) == fseeds.end()) {
			_ptiddata ptd = (_ptiddata)FlsGetValue(dwFlsIndex);
			debuglog(LCF_THREAD,"FlsSetValue(%d,lpFlsData), set _tiddata structure at %08X",dwFlsIndex,ptd);
			cmdprintf("WATCH: %08X,d,u,AutoRandSeed_Fiber_%d",&(ptd->_holdrand),dwFlsIndex);
			fseeds[dwFlsIndex] = &(ptd->_holdrand);
		}
		FlsRecursing = FALSE;
	}
	return rv;
}
BOOL TlsRecursing = FALSE;
std::map<DWORD,DWORD *> tseeds;
HOOKFUNC BOOL WINAPI MyTlsSetValue(DWORD dwTlsIndex, LPVOID lpTlsValue) {
	BOOL rv = TlsSetValue(dwTlsIndex, lpTlsValue);
	if ((!TlsRecursing) && (lpTlsValue != NULL)) {
		TlsRecursing = TRUE;
		if (tseeds.find(dwTlsIndex) == tseeds.end()) {
			_ptiddata ptd = (_ptiddata)TlsGetValue(dwTlsIndex);
			debuglog(LCF_THREAD,"TlsSetValue(%d,lpTlsValue), set _tiddata structure at %08X",dwTlsIndex,ptd);
			cmdprintf("WATCH: %08X,d,u,AutoRandSeed_Thread_%d",&(ptd->_holdrand),dwTlsIndex);
			tseeds[dwTlsIndex] = &(ptd->_holdrand);
		}
		TlsRecursing = FALSE;
	}
	return rv;
}

// not really hooked, I just needed their trampolines
HOOKFUNC LPVOID WINAPI MyTlsGetValue(DWORD dwTlsIndex) IMPOSSIBLE_IMPL
HOOKFUNC PVOID WINAPI MyFlsGetValue(DWORD dwFlsIndex) IMPOSSIBLE_IMPL


void ModuleDllMainInit()
{
	InitializeCriticalSection(&s_dllLoadAndRetryInterceptCS);
}

void ApplyModuleIntercepts()
{
	static const InterceptDescriptor intercepts [] = 
	{
		//MAKE_INTERCEPT(1, KERNEL32, GetProcAddress),
		//MAKE_INTERCEPT(1, KERNEL32, LoadLibraryExW),
		//MAKE_INTERCEPT(1, NTDLL, LdrUnloadDll),
		MAKE_INTERCEPT(1, NTDLL, LdrLoadDll),
		MAKE_INTERCEPT(1, NTDLL, KiUserCallbackDispatcher),

		MAKE_INTERCEPT(1, OLE32, CoCreateInstance),
		MAKE_INTERCEPT(1, OLE32, CoCreateInstanceEx),
		MAKE_INTERCEPT(1, OLE32, CoGetClassObject),
		MAKE_INTERCEPT3(1, QUARTZ.DLL, DllGetClassObject, quartz), // this is mainly so we can hook the IReferenceClock used by DirectShow
		MAKE_INTERCEPT(1, RPCRT4, IUnknown_QueryInterface_Proxy), // not sure if this is needed for anything

		MAKE_INTERCEPT(0, KERNEL32, FlsGetValue), // get trampoline only
		MAKE_INTERCEPT(0, KERNEL32, TlsGetValue), // get trampoline only
		MAKE_INTERCEPT(1, KERNEL32, FlsSetValue),
		MAKE_INTERCEPT(1, KERNEL32, TlsSetValue),

		MAKE_INTERCEPT(1, KERNEL32, ExitProcess),
		MAKE_INTERCEPT(1, KERNEL32, CreateProcessA),
		MAKE_INTERCEPT(1, KERNEL32, CreateProcessW),
		//MAKE_INTERCEPT(0, ADVAPI32, OpenServiceA),
		//MAKE_INTERCEPT(0, ADVAPI32, OpenServiceW),
		MAKE_INTERCEPT(1, NTDLL, RtlAllocateHeap),
		MAKE_INTERCEPT(1, NTDLL, RtlCreateHeap),
		MAKE_INTERCEPT(1, RPCRT4, NdrAllocate),
		MAKE_INTERCEPT(1, RPCRT4, NdrClientInitialize),
		MAKE_INTERCEPT(1, RPCRT4, NdrClientInitializeNew),
	};
	ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
}


#else
#pragma message(__FILE__": (skipped compilation)")
#endif

/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(EXTENDEDTRACE_C_INCL) && !defined(UNITY_BUILD)
#define EXTENDEDTRACE_C_INCL

//////////////////////////////////////////////////////////////////////////////////////
//
// Written by Zoltan Csizmadia, zoltan_csizmadia@yahoo.com
// For companies(Austin,TX): If you would like to get my resume, send an email.
//
// The source is free, but if you want to use it, mention my name and e-mail address
//
// nitsuja: I am using it. Thanks, Zoltan Csizmadia at zoltan_csizmadia@yahoo.com.
//          I modified it to print out an external process's call stack, and with parameter printing that actually works, and to be more tolerant of different dbghelp versions.
//////////////////////////////////////////////////////////////////////////////////////
//
// ExtendedTrace.cpp
//

// Include StdAfx.h, if you're using precompiled 
// header through StdAfx.h
//#include "stdafx.h"

bool traceEnabled = true;

#if 1//defined(_DEBUG) && defined(WIN32)

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _WIN32_WINNT 0x0500
#include <stdio.h>
#include <windows.h>
#include <tchar.h>
//#include <ImageHlp.h>
//#include <dbghelp.h>
#include "../../external/dbghelp.h"
#include "../../shared/msg.h"
#include "extendedtrace.h"

//#pragma comment(lib, "imagehlp.lib")
//#pragma comment(lib, "dbghelp.lib")

// this define is a HACK until a bug is fixed elsewhere.
// since the game can launch sub-processes,
// we need to pass the hProcess every time instead of having one global hProcess for the game,
// but apparently some of the code in DebuggerThreadFunc is passing the wrong hProcess
// so for now this define makes it work like the old global method again.
#define ASSUME_SINGLE_HPROCESS

#ifdef ASSUME_SINGLE_HPROCESS
static HANDLE s_hProcess = 0;
#endif


static BOOL(__stdcall *pSymInitialize)(HANDLE, PSTR, BOOL) = NULL;
static BOOL(__stdcall *pSymCleanup)(HANDLE) = NULL;
static DWORD(__stdcall *pSymSetOptions)(DWORD) = NULL;
static DWORD(__stdcall *pSymLoadModule)(HANDLE, HANDLE, PCSTR, PCSTR, DWORD, DWORD) = NULL;
static BOOL(__stdcall *pSymGetModuleInfo)(HANDLE, DWORD, PIMAGEHLP_MODULE) = NULL;
static PVOID(__stdcall *pImageRvaToVa)(PIMAGE_NT_HEADERS, PVOID, ULONG, PIMAGE_SECTION_HEADER*) = NULL;
static BOOL(__stdcall *pSymEnumSymbols)(HANDLE, ULONG64, PCSTR, PSYM_ENUMERATESYMBOLS_CALLBACK, PVOID) = NULL;
static BOOL(__stdcall *pStackWalk)(DWORD, HANDLE, HANDLE, LPSTACKFRAME, PVOID, PREAD_PROCESS_MEMORY_ROUTINE, PFUNCTION_TABLE_ACCESS_ROUTINE, PGET_MODULE_BASE_ROUTINE, PTRANSLATE_ADDRESS_ROUTINE) = NULL;
static PVOID(__stdcall *pSymFunctionTableAccess)(HANDLE, DWORD) = NULL;
static DWORD(__stdcall *pSymGetModuleBase)(HANDLE, DWORD) = NULL;
static BOOL(__stdcall *pSymFromAddr)(HANDLE, DWORD64, PDWORD64, PSYMBOL_INFO) = NULL;
static BOOL(__stdcall *pSymGetSymFromAddr)(HANDLE, DWORD, PDWORD, PIMAGEHLP_SYMBOL) = NULL;
static DWORD(__stdcall *pUnDecorateSymbolName)(PCSTR, PSTR, DWORD, DWORD) = NULL;
static BOOL(__stdcall *pSymGetLineFromAddr)(HANDLE hProcess, DWORD, PDWORD, PIMAGEHLP_LINE) = NULL;
static BOOL(__stdcall *pSymGetTypeInfo)(HANDLE, DWORD64, ULONG, IMAGEHLP_SYMBOL_TYPE_INFO, PVOID) = NULL;

void LoadDbghelpDll()
{
	if(!traceEnabled)
		return;

	static bool loaded = false;
	if(loaded)
		return;
	loaded = true;

	// try to load dbghelp from current or parent directory
	// since it's likely more up-to-date than the one in win32 directory
	// (microsoft recommends distributing dbghelp.dll and using it instead of the os version)
	char path [MAX_PATH+1+sizeof("dbghelp.dll")] = {0};
	GetModuleFileNameA(NULL, path, MAX_PATH);
	HMODULE dll = NULL;
	while(!dll)
	{
		char* slash = strrchr(path, '\\');
		char* next = slash ? slash+1 : path;
		strcpy(next, "dbghelp.dll");
		dll = LoadLibrary(path);
		if(!slash)
			break;
		*slash = 0;
	}
	if(!dll)
		return;

	// no error checking here because there is more granular handling of missing functions elsewhere,
	// so that we can still get partial functionality when using the wrong version of dbghelp.
	(FARPROC&)pSymInitialize = GetProcAddress(dll, "SymInitialize");
	(FARPROC&)pSymCleanup = GetProcAddress(dll, "SymCleanup");
	(FARPROC&)pSymSetOptions = GetProcAddress(dll, "SymSetOptions");
	(FARPROC&)pSymLoadModule = GetProcAddress(dll, "SymLoadModule");
	(FARPROC&)pSymGetModuleInfo = GetProcAddress(dll, "SymGetModuleInfo");
	(FARPROC&)pImageRvaToVa = GetProcAddress(dll, "ImageRvaToVa");
	(FARPROC&)pSymEnumSymbols = GetProcAddress(dll, "SymEnumSymbols");
	(FARPROC&)pStackWalk = GetProcAddress(dll, "StackWalk");
	(FARPROC&)pSymFunctionTableAccess = GetProcAddress(dll, "SymFunctionTableAccess");
	(FARPROC&)pSymGetModuleBase = GetProcAddress(dll, "SymGetModuleBase");
	(FARPROC&)pSymFromAddr = GetProcAddress(dll, "SymFromAddr");
	(FARPROC&)pSymGetSymFromAddr = GetProcAddress(dll, "SymGetSymFromAddr");
	(FARPROC&)pUnDecorateSymbolName = GetProcAddress(dll, "UnDecorateSymbolName");
	(FARPROC&)pSymGetLineFromAddr = GetProcAddress(dll, "SymGetLineFromAddr");
	(FARPROC&)pSymGetTypeInfo = GetProcAddress(dll, "SymGetTypeInfo");
}





#define BUFFERSIZE   0x800

#if defined(_AFX) || defined(_AFXDLL)
// I don't wanna be smart :)
#define OutputDebugStringFormat	::AfxTrace
#else
// Implement Win32 TRACE(...)
int debugprintf(const char * fmt, ...);
#define OutputDebugStringFormat debugprintf
#endif // OutputDebugStringFormat

// Unicode safe char* -> TCHAR* conversion
void PCSTR2LPTSTR( PCSTR lpszIn, LPTSTR lpszOut )
{
#if defined(UNICODE)||defined(_UNICODE)
   ULONG index = 0; 
   PCSTR lpAct = lpszIn;
   
	for( ; ; lpAct++ )
	{
		lpszOut[index++] = (TCHAR)(*lpAct);
		if ( *lpAct == 0 )
			break;
	} 
#else
   // This is trivial :)
	strcpy( lpszOut, lpszIn );
#endif
}

// Let's figure out the path for the symbol files
// Search path= ".;%_NT_SYMBOL_PATH%;%_NT_ALTERNATE_SYMBOL_PATH%;%SYSTEMROOT%;%SYSTEMROOT%\System32;" + lpszIniPath
// Note: There is no size check for lpszSymbolPath!
void InitSymbolPath( PSTR lpszSymbolPath, PCSTR lpszIniPath )
{
	CHAR lpszPath[BUFFERSIZE];

   // Creating the default path
   // ".;%_NT_SYMBOL_PATH%;%_NT_ALTERNATE_SYMBOL_PATH%;%SYSTEMROOT%;%SYSTEMROOT%\System32;"
	strcpy( lpszSymbolPath, ".;" );
	GetCurrentDirectoryA(BUFFERSIZE-2, lpszSymbolPath+2); // in case it changes somehow
//	strcat( lpszSymbolPath, ";" );

	// environment variable _NT_SYMBOL_PATH
	if ( GetEnvironmentVariableA( "_NT_SYMBOL_PATH", lpszPath, BUFFERSIZE ) )
	{
	   strcat( lpszSymbolPath, ";" );
		strcat( lpszSymbolPath, lpszPath );
	}

	// environment variable _NT_ALTERNATE_SYMBOL_PATH
	if ( GetEnvironmentVariableA( "_NT_ALTERNATE_SYMBOL_PATH", lpszPath, BUFFERSIZE ) )
	{
	   strcat( lpszSymbolPath, ";" );
		strcat( lpszSymbolPath, lpszPath );
	}

	// environment variable SYSTEMROOT
	if ( GetEnvironmentVariableA( "SYSTEMROOT", lpszPath, BUFFERSIZE ) )
	{
	   strcat( lpszSymbolPath, ";" );
		strcat( lpszSymbolPath, lpszPath );

		// SYSTEMROOT\System32
		strcat( lpszSymbolPath, ";" );
		strcat( lpszSymbolPath, lpszPath );
		strcat( lpszSymbolPath, "\\System32" );

		// SYSTEMROOT\Symbols\dll
		strcat( lpszSymbolPath, ";" );
		strcat( lpszSymbolPath, lpszPath );
		strcat( lpszSymbolPath, "\\Symbols\\dll" );
	}

   // Add user defined path
	if ( lpszIniPath != NULL )
		if ( lpszIniPath[0] != '\0' )
		{
		   strcat( lpszSymbolPath, ";" );
			strcat( lpszSymbolPath, lpszIniPath );
		}
}

// Uninitialize the loaded symbol files
BOOL UninitSymInfo(HANDLE hProcess)
{
#ifdef ASSUME_SINGLE_HPROCESS
	hProcess = s_hProcess;
#endif
	return pSymCleanup ? pSymCleanup( hProcess ) : FALSE;
}

void* RVAToPointer( DWORD rva, LOADED_IMAGE& Image )
{
  PIMAGE_SECTION_HEADER* dummy = 0;
  return pImageRvaToVa ? pImageRvaToVa( Image.FileHeader, Image.MappedAddress, rva, dummy ) : NULL;
}

BOOL CALLBACK EnumSymProc( 
    PSYMBOL_INFO pSymInfo,   
    ULONG SymbolSize,      
    PVOID UserContext)
{
#if defined(_DEBUG) && 0
    debugprintf("0x%X %s\n", (DWORD)pSymInfo->Address, pSymInfo->Name);
#endif
    return TRUE;
}


void LoadModuleSymbols(HANDLE hProcess, PSTR name)
{
#ifdef ASSUME_SINGLE_HPROCESS
	hProcess = s_hProcess;
#endif

	DWORD dwBaseAddress = pSymLoadModule ? pSymLoadModule( hProcess, 0, name, 0, 0, 0 ) : NULL;
	IMAGEHLP_MODULE im = { sizeof(IMAGEHLP_MODULE) };
	if(pSymGetModuleInfo)
	{
		pSymGetModuleInfo( hProcess, dwBaseAddress, &im );

		OutputDebugStringFormat(
			"LoadModuleSymbols(0x%X, %s) = {BaseOfImage=0x%X, NumSyms=%d, SymType=0x%X, ModuleName=%s, ImageName=%s}\n",
			(int)hProcess, (const char*)name, (int)im.BaseOfImage, (int)im.NumSyms, (int)im.SymType, (const char*)im.ModuleName, (const char*)im.ImageName
		);
	}

	if(pSymEnumSymbols)
		pSymEnumSymbols(hProcess, dwBaseAddress, NULL, EnumSymProc, NULL);




#if 0
	PSTR pszModName;
	ULONG ulSize;
	IMAGE_EXPORT_DIRECTORY* exportDirectory;

	LOADED_IMAGE imageinfo;

	if(exportDirectory = (IMAGE_EXPORT_DIRECTORY*)ImageDirectoryEntryToData((void*)dwBaseAddress, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &ulSize))
	{
		if(MapAndLoad( name, NULL, &imageinfo, true, true ))
		{
			DWORD* nameRVAs = (DWORD*)RVAToPointer( DWORD(exportDirectory->AddressOfNames), imageinfo ); 
			//DWORD* functionRVAs = (DWORD*)RVAToPointer( DWORD(exportDirectory->AddressOfFunctions), imageinfo ); 
			for(int i = 0; i < exportDirectory->NumberOfNames; i++)
			{
				char* functionName = (char*)RVAToPointer(nameRVAs[i], imageinfo);
				//void* functionAddress = RVAToPointer(functionRVAs[i], imageinfo);
				OutputDebugString(functionName);
				OutputDebugString("\n");
			}

			UnMapAndLoad(&imageinfo);
		}
	}
#endif

	return;
}

void LoadModuleSymbols(HANDLE hProcess, PSTR name, HANDLE file, DWORD base )
{
#ifdef ASSUME_SINGLE_HPROCESS
	hProcess = s_hProcess;
#endif
	DWORD dwBaseAddress = pSymLoadModule ? pSymLoadModule( hProcess, file, name, 0, base, 0 ) : NULL;
	IMAGEHLP_MODULE im = { sizeof(IMAGEHLP_MODULE) };
	if(pSymGetModuleInfo)
	{
		pSymGetModuleInfo( hProcess, dwBaseAddress, &im );

		OutputDebugStringFormat(
			"LoadModuleSymbols(0x%X, %s, 0x%X, 0x%X) = {BaseOfImage=0x%X, NumSyms=%d, SymType=0x%X, ModuleName=%s, ImageName=%s}\n",
			(int)hProcess, name, (DWORD)file, (DWORD)base, (int)im.BaseOfImage, (int)im.NumSyms, (int)im.SymType, (const char*)im.ModuleName, (const char*)im.ImageName
		);
	}

	if(pSymEnumSymbols)
		pSymEnumSymbols(hProcess, dwBaseAddress, NULL, EnumSymProc, NULL);
}



// Initializes the symbol files
BOOL InitSymInfo( PCSTR lpszInitialSymbolPath, HANDLE hProcess )
{
	LoadDbghelpDll();

	if(!pSymInitialize || !traceEnabled)
		return FALSE;

#ifdef ASSUME_SINGLE_HPROCESS
	s_hProcess = hProcess;
#endif

	CHAR     lpszSymbolPath[BUFFERSIZE];
//    DWORD    symOptions;
   
	DWORD    symOptions = 0;
	symOptions |= SYMOPT_LOAD_LINES; 
	symOptions |= SYMOPT_LOAD_ANYTHING; 
	symOptions |= SYMOPT_CASE_INSENSITIVE;
	symOptions |= SYMOPT_OMAP_FIND_NEAREST;
	symOptions |= SYMOPT_ALLOW_ABSOLUTE_SYMBOLS;
	//symOptions |= SYMOPT_UNDNAME;
	//symOptions |= SYMOPT_NO_PROMPTS;
	if(pSymSetOptions)
		pSymSetOptions( symOptions );

   // Get the search path for the symbol files
	InitSymbolPath( lpszSymbolPath, lpszInitialSymbolPath );
	debugprintf("SYMBOL PATH: %s\n", lpszSymbolPath);
 //   symOptions = SymGetOptions();
	//symOptions |= SYMOPT_LOAD_LINES; 
	//symOptions &= ~SYMOPT_UNDNAME;
	//pSymSetOptions( symOptions );

	BOOL rv = pSymInitialize( hProcess, lpszSymbolPath, TRUE);
	if(!rv) // pSymInitialize fails on Vista if the last parameter is TRUE, so try again:
		rv = pSymInitialize( hProcess, lpszSymbolPath, FALSE);
	if(!rv) // try without a search path?
		rv = pSymInitialize( hProcess, NULL, FALSE);
	
	return rv;
}

// Get the module name from a given address
BOOL GetModuleNameFromAddress( UINT address, LPTSTR lpszModule, HANDLE hProcess )
{
#ifdef ASSUME_SINGLE_HPROCESS
	hProcess = s_hProcess;
#endif
	BOOL              ret = FALSE;

	// SymGetModuleInfo64 doesn't work for me... maybe I'm including the wrong dbghelp.h
	//IMAGEHLP_MODULE64 moduleInfo = { sizeof(IMAGEHLP_MODULE64) };
	//pSymGetModuleInfo64( hProcess, (DWORD64)address, &moduleInfo );
	//if ( pSymGetModuleInfo64( hProcess, (DWORD64)address, &moduleInfo ) )

	IMAGEHLP_MODULE moduleInfo = { sizeof(IMAGEHLP_MODULE) };
	moduleInfo.SizeOfStruct = sizeof(moduleInfo);
	if ( pSymGetModuleInfo && pSymGetModuleInfo( hProcess, (DWORD)address, &moduleInfo ) )
	{
	   // Got it!
		PCSTR2LPTSTR( moduleInfo.ModuleName, lpszModule );
		ret = TRUE;
	}
	else
	   // Not found :(
		_tcscpy( lpszModule, _T("?") );
	
	return ret;
}

inline bool isLikelyStringChar(char c)
{
	return c >= 32 && c <= 127;
}

// TODO: move to shared
static const char* GetWindowsMessageName(UINT message)
{
	switch(message)
	{
#define GWMNcase(wm) case wm: return #wm;
		GWMNcase(WM_NULL)
		GWMNcase(WM_PAINT)
		GWMNcase(WM_CREATE)
		GWMNcase(WM_DESTROY)
		GWMNcase(WM_MOVE)
		GWMNcase(WM_SIZE)
		GWMNcase(WM_ACTIVATE)
		GWMNcase(WM_SETFOCUS)
		GWMNcase(WM_KILLFOCUS)
		GWMNcase(WM_ENABLE)
		GWMNcase(WM_SETREDRAW)
		GWMNcase(WM_SETTEXT)
		GWMNcase(WM_GETTEXT)
		GWMNcase(WM_GETTEXTLENGTH)
		GWMNcase(WM_CLOSE)
		GWMNcase(WM_QUERYENDSESSION)
		GWMNcase(WM_QUERYOPEN)
		GWMNcase(WM_ENDSESSION)
		GWMNcase(WM_QUIT)
		GWMNcase(WM_ERASEBKGND)
		GWMNcase(WM_SYSCOLORCHANGE)
		GWMNcase(WM_SHOWWINDOW)
		GWMNcase(WM_WININICHANGE)
		GWMNcase(WM_DEVMODECHANGE)
		GWMNcase(WM_ACTIVATEAPP)
		GWMNcase(WM_FONTCHANGE)
		GWMNcase(WM_TIMECHANGE)
		GWMNcase(WM_CANCELMODE)
		GWMNcase(WM_SETCURSOR)
		GWMNcase(WM_MOUSEACTIVATE)
		GWMNcase(WM_CHILDACTIVATE)
		GWMNcase(WM_QUEUESYNC)
		GWMNcase(WM_GETMINMAXINFO)
		GWMNcase(WM_PAINTICON)
		GWMNcase(WM_ICONERASEBKGND)
		GWMNcase(WM_NEXTDLGCTL)
		GWMNcase(WM_SPOOLERSTATUS)
		GWMNcase(WM_DRAWITEM)
		GWMNcase(WM_MEASUREITEM)
		GWMNcase(WM_DELETEITEM)
		GWMNcase(WM_VKEYTOITEM)
		GWMNcase(WM_CHARTOITEM)
		GWMNcase(WM_SETFONT)
		GWMNcase(WM_GETFONT)
		GWMNcase(WM_SETHOTKEY)
		GWMNcase(WM_GETHOTKEY)
		GWMNcase(WM_QUERYDRAGICON)
		GWMNcase(WM_COMPAREITEM)
		GWMNcase(WM_GETOBJECT)
		GWMNcase(WM_COMPACTING)
		GWMNcase(WM_COMMNOTIFY)
		GWMNcase(WM_WINDOWPOSCHANGING)
		GWMNcase(WM_WINDOWPOSCHANGED)
		GWMNcase(WM_POWER)
		GWMNcase(WM_COPYDATA)
		GWMNcase(WM_CANCELJOURNAL)
		GWMNcase(WM_NOTIFY)
		GWMNcase(WM_INPUTLANGCHANGEREQUEST)
		GWMNcase(WM_INPUTLANGCHANGE)
		GWMNcase(WM_TCARD)
		GWMNcase(WM_HELP)
		GWMNcase(WM_USERCHANGED)
		GWMNcase(WM_NOTIFYFORMAT)
		GWMNcase(WM_CONTEXTMENU)
		GWMNcase(WM_STYLECHANGING)
		GWMNcase(WM_STYLECHANGED)
		GWMNcase(WM_DISPLAYCHANGE)
		GWMNcase(WM_GETICON)
		GWMNcase(WM_SETICON)
		GWMNcase(WM_NCCREATE)
		GWMNcase(WM_NCDESTROY)
		GWMNcase(WM_NCCALCSIZE)
		GWMNcase(WM_NCHITTEST)
		GWMNcase(WM_NCPAINT)
		GWMNcase(WM_NCACTIVATE)
		GWMNcase(WM_GETDLGCODE)
		GWMNcase(WM_SYNCPAINT)
		GWMNcase(WM_NCMOUSEMOVE)
		GWMNcase(WM_NCLBUTTONDOWN)
		GWMNcase(WM_NCLBUTTONUP)
		GWMNcase(WM_NCLBUTTONDBLCLK)
		GWMNcase(WM_NCRBUTTONDOWN)
		GWMNcase(WM_NCRBUTTONUP)
		GWMNcase(WM_NCRBUTTONDBLCLK)
		GWMNcase(WM_NCMBUTTONDOWN)
		GWMNcase(WM_NCMBUTTONUP)
		GWMNcase(WM_NCMBUTTONDBLCLK)
		GWMNcase(WM_NCXBUTTONDOWN)
		GWMNcase(WM_NCXBUTTONUP)
		GWMNcase(WM_NCXBUTTONDBLCLK)
		case /*WM_NCUAHDRAWCAPTION*/0xAE: return "WM_NCUAHDRAWCAPTION";
		case /*WM_NCUAHDRAWFRAME*/0xAF: return "WM_NCUAHDRAWFRAME";
		case /*WM_INPUT_DEVICE_CHANGE*/0x00FE: return "WM_INPUT_DEVICE_CHANGE";
		case /*WM_INPUT*/0x00FF: return "WM_INPUT";
		GWMNcase(WM_KEYDOWN)
		GWMNcase(WM_KEYUP)
		GWMNcase(WM_CHAR)
		GWMNcase(WM_DEADCHAR)
		GWMNcase(WM_SYSKEYDOWN)
		GWMNcase(WM_SYSKEYUP)
		GWMNcase(WM_SYSCHAR)
		GWMNcase(WM_SYSDEADCHAR)
		case /*WM_UNICHAR*/0x0109: return "WM_UNICHAR";
		GWMNcase(WM_IME_STARTCOMPOSITION)
		GWMNcase(WM_IME_ENDCOMPOSITION)
		GWMNcase(WM_IME_COMPOSITION)
		GWMNcase(WM_INITDIALOG)
		GWMNcase(WM_COMMAND)
		GWMNcase(WM_SYSCOMMAND)
		GWMNcase(WM_TIMER)
		GWMNcase(WM_HSCROLL)
		GWMNcase(WM_VSCROLL)
		GWMNcase(WM_INITMENU)
		GWMNcase(WM_INITMENUPOPUP)
		case /*WM_SYSTIMER*/0x0118: return "WM_SYSTIMER";
		GWMNcase(WM_MENUSELECT)
		GWMNcase(WM_MENUCHAR)
		GWMNcase(WM_ENTERIDLE)
		GWMNcase(WM_MENURBUTTONUP)
		GWMNcase(WM_MENUDRAG)
		GWMNcase(WM_MENUGETOBJECT)
		GWMNcase(WM_UNINITMENUPOPUP)
		GWMNcase(WM_MENUCOMMAND)
		GWMNcase(WM_CHANGEUISTATE)
		GWMNcase(WM_UPDATEUISTATE)
		GWMNcase(WM_QUERYUISTATE)
		GWMNcase(WM_CTLCOLORMSGBOX)
		GWMNcase(WM_CTLCOLOREDIT)
		GWMNcase(WM_CTLCOLORLISTBOX)
		GWMNcase(WM_CTLCOLORBTN)
		GWMNcase(WM_CTLCOLORDLG)
		GWMNcase(WM_CTLCOLORSCROLLBAR)
		GWMNcase(WM_CTLCOLORSTATIC)
		GWMNcase(WM_MOUSEMOVE)
		GWMNcase(WM_LBUTTONDOWN)
		GWMNcase(WM_LBUTTONUP)
		GWMNcase(WM_LBUTTONDBLCLK)
		GWMNcase(WM_RBUTTONDOWN)
		GWMNcase(WM_RBUTTONUP)
		GWMNcase(WM_RBUTTONDBLCLK)
		GWMNcase(WM_MBUTTONDOWN)
		GWMNcase(WM_MBUTTONUP)
		GWMNcase(WM_MBUTTONDBLCLK)
		GWMNcase(WM_MOUSEWHEEL)
		GWMNcase(WM_XBUTTONDOWN)
		GWMNcase(WM_XBUTTONUP)
		GWMNcase(WM_XBUTTONDBLCLK)
		case /*WM_MOUSEHWHEEL*/0x020E: return "WM_MOUSEHWHEEL";
		GWMNcase(WM_PARENTNOTIFY)
		GWMNcase(WM_ENTERMENULOOP)
		GWMNcase(WM_EXITMENULOOP)
		GWMNcase(WM_NEXTMENU)
		GWMNcase(WM_SIZING)
		GWMNcase(WM_CAPTURECHANGED)
		GWMNcase(WM_MOVING)
		GWMNcase(WM_POWERBROADCAST)
		GWMNcase(WM_DEVICECHANGE)
		GWMNcase(WM_MDICREATE)
		GWMNcase(WM_MDIDESTROY)
		GWMNcase(WM_MDIACTIVATE)
		GWMNcase(WM_MDIRESTORE)
		GWMNcase(WM_MDINEXT)
		GWMNcase(WM_MDIMAXIMIZE)
		GWMNcase(WM_MDITILE)
		GWMNcase(WM_MDICASCADE)
		GWMNcase(WM_MDIICONARRANGE)
		GWMNcase(WM_MDIGETACTIVE)
		GWMNcase(WM_MDISETMENU)
		GWMNcase(WM_ENTERSIZEMOVE)
		GWMNcase(WM_EXITSIZEMOVE)
		GWMNcase(WM_DROPFILES)
		GWMNcase(WM_MDIREFRESHMENU)
		GWMNcase(WM_IME_SETCONTEXT)
		GWMNcase(WM_IME_NOTIFY)
		GWMNcase(WM_IME_CONTROL)
		GWMNcase(WM_IME_COMPOSITIONFULL)
		GWMNcase(WM_IME_SELECT)
		GWMNcase(WM_IME_CHAR)
		case /*WM_IME_SYSTEM*/0x0287: return "WM_IME_SYSTEM";
		GWMNcase(WM_IME_REQUEST)
		GWMNcase(WM_IME_KEYDOWN)
		GWMNcase(WM_IME_KEYUP)
		GWMNcase(WM_MOUSEHOVER)
		GWMNcase(WM_MOUSELEAVE)
		GWMNcase(WM_NCMOUSEHOVER)
		GWMNcase(WM_NCMOUSELEAVE)
		GWMNcase(WM_CUT)
		GWMNcase(WM_COPY)
		GWMNcase(WM_PASTE)
		GWMNcase(WM_CLEAR)
		GWMNcase(WM_UNDO)
		GWMNcase(WM_RENDERFORMAT)
		GWMNcase(WM_RENDERALLFORMATS)
		GWMNcase(WM_DESTROYCLIPBOARD)
		GWMNcase(WM_DRAWCLIPBOARD)
		GWMNcase(WM_PAINTCLIPBOARD)
		GWMNcase(WM_VSCROLLCLIPBOARD)
		GWMNcase(WM_SIZECLIPBOARD)
		GWMNcase(WM_ASKCBFORMATNAME)
		GWMNcase(WM_CHANGECBCHAIN)
		GWMNcase(WM_HSCROLLCLIPBOARD)
		GWMNcase(WM_QUERYNEWPALETTE)
		GWMNcase(WM_PALETTEISCHANGING)
		GWMNcase(WM_PALETTECHANGED)
		GWMNcase(WM_HOTKEY)
		case /*WM_POPUPSYSTEMMENU*/0x0313: return "WM_POPUPSYSTEMMENU";
		GWMNcase(WM_PRINT)
		GWMNcase(WM_PRINTCLIENT)
		GWMNcase(WM_APPCOMMAND)
		case /*WM_CLIPBOARDUPDATE*/0x031D: return "WM_CLIPBOARDUPDATE";
		case /*WM_GETTITLEBARINFOEX*/0x033F: return "WM_GETTITLEBARINFOEX";
		GWMNcase(WM_APP)
#undef GWMNcase
		default: 
		{
			if(isMessageWhitelisted(message))
				return GetWindowsMessageName(toggleWhitelistMessage(message));
			if(message >= 0xC000 && message <= 0xFFFF)
			{
				static char temp [64];
				if(GetClipboardFormatNameA(message,temp,ARRAYSIZE(temp))) // hack since NtUserGetAtomName doesn't seem to work for this
					return temp;
			}

			static char rvtemp [16];
			sprintf(rvtemp, "0x%X", message);
			return rvtemp;
		}
	}
}


// Get function prototype and parameter info from ip address and stack address
BOOL GetFunctionInfoFromAddresses( ULONG fnAddress, ULONG stackAddress, LPTSTR lpszSymbol/*[BUFFERSIZE]*/, HANDLE hProcess )
{
#ifdef ASSUME_SINGLE_HPROCESS
	hProcess = s_hProcess;
#endif
	BOOL ret = FALSE;
	DWORD64 dwDisp = 0;
	DWORD dwSymSize = MAX_SYM_NAME + sizeof(SYMBOL_INFO);
	CHAR lpszUnDSymbol[MAX_SYM_NAME]="?";
	PSYMBOL_INFO pSym = (PSYMBOL_INFO)GlobalAlloc( GMEM_FIXED, dwSymSize );

	ZeroMemory(pSym, dwSymSize);
	pSym->SizeOfStruct = sizeof(SYMBOL_INFO);
	pSym->MaxNameLen = MAX_SYM_NAME;
//	pSym->Flags = SYMFLAG_FUNCTION;

	// Get symbol info for IP

	bool gotSymbol;
	__try {
		gotSymbol = pSymFromAddr && pSymFromAddr(hProcess, (DWORD64)fnAddress, &dwDisp, pSym);
	} __except(EXCEPTION_EXECUTE_HANDLER) {
		gotSymbol = false;
	}

	if(!gotSymbol)
	{
		// maybe SymFromAddr is missing. try the older version, SymGetSymFromAddr
		DWORD dwDisp32;
		DWORD dwSymSize32 = MAX_SYM_NAME + sizeof(IMAGEHLP_SYMBOL);
		PIMAGEHLP_SYMBOL pImSym = (PIMAGEHLP_SYMBOL)GlobalAlloc( GMEM_FIXED, dwSymSize );
		ZeroMemory(pImSym, dwSymSize32);
		pImSym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
		pImSym->MaxNameLength = MAX_SYM_NAME;
		gotSymbol = pSymGetSymFromAddr && pSymGetSymFromAddr(hProcess, (DWORD)fnAddress, &dwDisp32, pImSym);
		if(gotSymbol)
		{
			pSym->Address = pImSym->Address;
			pSym->Flags = pImSym->Flags;
			pSym->Size = pImSym->Size;
			strcpy(pSym->Name, pImSym->Name);
		}
	}

	if(gotSymbol)
	{
		ret = TRUE;
		if(pUnDecorateSymbolName)
		{
			// Make the symbol readable for humans
			pUnDecorateSymbolName(pSym->Name, lpszUnDSymbol, MAX_SYM_NAME, 
				UNDNAME_COMPLETE 
			|   UNDNAME_NO_THISTYPE
			|   UNDNAME_NO_SPECIAL_SYMS
			|   UNDNAME_NO_MEMBER_TYPE
			|   UNDNAME_NO_MS_KEYWORDS
			|   UNDNAME_NO_ACCESS_SPECIFIERS 
			);

			strcpy(lpszSymbol, lpszUnDSymbol);
		}
		else
		{
			strcpy(lpszSymbol, pSym->Name);
		}
	}
	else
	{
		strcpy(lpszSymbol, "? ");
	}

	CHAR* pOutString = lpszSymbol + strlen(lpszSymbol);

	DWORD dwChildrenCount = 0;
	BOOL hasTypeInfo = pSymGetTypeInfo && pSymGetTypeInfo( hProcess, pSym->ModBase, pSym->TypeIndex, TI_GET_CHILDRENCOUNT, &dwChildrenCount );

	int nargs = 4; // if no type info, just guess and print out 4 arguments
	bool skipArgList = false;
	bool unsure = true;

	if(hasTypeInfo)
	{
		unsure = false;
		nargs = dwChildrenCount;
	}
	else
	{
		// sometimes we can guess the parameter count from the undecorated symbol name (if it's a C++ class method)
		if(strchr(lpszSymbol, '('))
		{
			unsure = false;
			if(strstr(lpszSymbol, "(void)") || strstr(lpszSymbol, "()"))
				skipArgList = true;
			else
			{
				pOutString += sprintf(pOutString, " ");
				int numCommas = 0;
				char* lpszSymbol2 = lpszSymbol;
				while(*lpszSymbol2)
				{
					if(*lpszSymbol2 == ',')
						numCommas++;
					lpszSymbol2++;
				}
				nargs = numCommas + 1;
				if(strstr(lpszSymbol, "::")) // first might be the this pointer
					nargs++;
			}
		}
		else if(!strcmp(lpszSymbol, "`string'"))
		{
			strcpy(lpszSymbol, "[entrypoint]");
			skipArgList = true;
		}
	}


	if(!skipArgList && stackAddress)
	{
		bool isWndProcFunc = strstr(lpszSymbol, "WndProc") || strstr(lpszSymbol, "WinProc") || strstr(lpszSymbol, "WindowProc") || strstr(lpszSymbol, "DispatchClientMessage") || (strstr(lpszSymbol, "SendMessage") && !strstr(lpszSymbol, "SendMessageWorker"));
		bool nextMightBeMessage = isWndProcFunc;

		pOutString += sprintf(pOutString, "(");
		bool anyArgs = false;
		for(int i = 0; i < nargs; i++)
		{
			void* remoteArgPtr = ((DWORD*)stackAddress) + 2 + i;
			DWORD value = 0;
			SIZE_T bytesRead = 0;
			if(ReadProcessMemory(hProcess, remoteArgPtr, &value, sizeof(value), &bytesRead))
			{
				if(value == 0xcccccccc && unsure)
					break;

				bool isSmallInt = ((int)value <= 1024 && (int)value >= -1024);
				bool isMultipleOf100 = ((int)value % 100) == 0 && (value <= 1000000);
				bool isString = false;
				bool isWmCode = isWndProcFunc && (i >= 1) && nextMightBeMessage;
				char dataShort [4] = {};
				char dataLong [256];
				dataLong[0] = 0;
				if(nextMightBeMessage && value && value < 0x040000)
					nextMightBeMessage = false;

				// try to detect whether it's a pointer to an ANSI string
				if(!isSmallInt && !isWndProcFunc && ReadProcessMemory(hProcess, (VOID*)value, &dataShort, sizeof(dataShort), &bytesRead))
				{
					isString = true;
					for(int j = 0; j < sizeof(dataShort); j++)
					{
						if(!dataShort[j])
							break;
						if(!isLikelyStringChar(dataShort[j]))
							isString = false;
					}
					if(isString)
					{
						isString = false;
						if(ReadProcessMemory(hProcess, (VOID*)value, &dataLong, sizeof(dataLong), &bytesRead))
						{
							isString = true;
							int k;
							for(k = 0; k < sizeof(dataLong); k++)
							{
								if(!dataLong[k])
									break;
								if(!isLikelyStringChar(dataLong[k]))
								{
									isString = false;
									break;
								}
							}
							if(k < 4 || k == sizeof(dataLong)) // if too short or too long
								isString = false;
						}
					}
				}

				if(anyArgs)
					pOutString += sprintf(pOutString, ", ");

				if(isString)
					pOutString += sprintf(pOutString, "0x%X (\"%s\")", value, dataLong);
				else if(isWmCode)
					pOutString += sprintf(pOutString, "0x%X (%s)", value, GetWindowsMessageName(value));
				else if(isSmallInt || isMultipleOf100)
					pOutString += sprintf(pOutString, "%d", (int)value);
				else
					pOutString += sprintf(pOutString, "0x%X", value);

				anyArgs = true;
			}
		}
		pOutString += sprintf(pOutString, unsure ? ")?" : ")");
	}

	GlobalFree( pSym );

	return ret;
}

// Get source file name and line number from IP address
// The output format is: "sourcefile(linenumber)" or
//                       "modulename!address" or
//                       "address"
BOOL GetSourceInfoFromAddress( UINT address, LPTSTR lpszSourceInfo, HANDLE hProcess )
{
#ifdef ASSUME_SINGLE_HPROCESS
	hProcess = s_hProcess;
#endif
	BOOL           ret = FALSE;
	IMAGEHLP_LINE  lineInfo;
	DWORD          dwDisp;
	TCHAR          lpszFileName[BUFFERSIZE] = _T("");
	TCHAR          lpModuleInfo[BUFFERSIZE] = _T("");

	_tcscpy( lpszSourceInfo, _T("?(?)") );

	::ZeroMemory( &lineInfo, sizeof( lineInfo ) );
	lineInfo.SizeOfStruct = sizeof( lineInfo );

	if ( pSymGetLineFromAddr && pSymGetLineFromAddr( hProcess, address, &dwDisp, &lineInfo ) )
	{
	   // Got it. Let's use "sourcefile(linenumber)" format
		PCSTR2LPTSTR( lineInfo.FileName, lpszFileName );
		_stprintf( lpszSourceInfo, _T("%s(%d)"), lpszFileName, lineInfo.LineNumber );
		//_stprintf( lpszSourceInfo, _T("wintasee!0x%08X, " "%s(%d)"), address, lpszFileName, lineInfo.LineNumber );
		ret = TRUE;
	}
	else
	{
      // There is no source file information. :(
      // Let's use the "modulename!address" format
	  	GetModuleNameFromAddress( address, lpModuleInfo, hProcess );

		if ( lpModuleInfo[0] == _T('?') || lpModuleInfo[0] == _T('\0'))
		   // There is no modulename information. :((
         // Let's use the "address" format
			_stprintf( lpszSourceInfo, _T("0x%08X"), address );
		else
			_stprintf( lpszSourceInfo, _T("%s!0x%08X"), lpModuleInfo, address );

		ret = FALSE;
	}
	
	return ret;
}

// TRACE message with source link. 
// The format is: sourcefile(linenumber) : message
void SrcLinkTrace( LPCTSTR lpszMessage, LPCTSTR lpszFileName, ULONG nLineNumber )
{
	OutputDebugStringFormat( 
			_T("%s(%d) : %s"), 
			lpszFileName, 
			nLineNumber, 
			lpszMessage );
}

//void TraceSingleAddress(DWORD addr, HANDLE hProcess)
//{
//	TCHAR symInfo[BUFFERSIZE] = _T("?");
//	TCHAR srcInfo[BUFFERSIZE] = _T("?");
//	GetFunctionInfoFromAddresses( addr, 0, symInfo, hProcess );
//	GetSourceInfoFromAddress( addr, srcInfo, hProcess );
//	OutputDebugStringFormat( _T("     %s : %s\n"), srcInfo, symInfo );
//}

void StackTraceOfDepth( HANDLE hThread, LPCTSTR lpszMessage, int minDepth, int maxDepth, HANDLE hProcess )
{
#ifdef ASSUME_SINGLE_HPROCESS
	hProcess = s_hProcess;
#endif
	if(!pStackWalk)
		return;

	STACKFRAME     callStack;
	BOOL           bResult;
	CONTEXT        context;
	TCHAR          symInfo[BUFFERSIZE] = _T("?");
	TCHAR          srcInfo[BUFFERSIZE] = _T("?");

	::ZeroMemory( &context, sizeof(context) );
	context.ContextFlags = CONTEXT_FULL;

	if ( !GetThreadContext( hThread, &context ) )
	{
      OutputDebugStringFormat( _T("Call stack info(thread=0x%X) failed.\n") );
	   return;
	}
	
	::ZeroMemory( &callStack, sizeof(callStack) );
	callStack.AddrPC.Offset    = context.Eip;
	callStack.AddrStack.Offset = context.Esp;
	callStack.AddrFrame.Offset = context.Ebp;
	callStack.AddrPC.Mode      = AddrModeFlat;
	callStack.AddrStack.Mode   = AddrModeFlat;
	callStack.AddrFrame.Mode   = AddrModeFlat;

   OutputDebugStringFormat( _T("Call stack info(thread=0x%X) : %s\n"), 
         hThread,
         lpszMessage );

	int failures = 0;

	for( ULONG index = 0; ; index++ ) 
	{
		bResult = pStackWalk(
			IMAGE_FILE_MACHINE_I386,
			hProcess,
			hThread,
	        &callStack,
			&context, 
			NULL,
			pSymFunctionTableAccess,
			pSymGetModuleBase,
			NULL);

//		debugprintf("AddrPC=0x%X, AddrReturn=0x%X, AddrFrame=0x%X, AddrStack=0x%X, FuncTableEntry=0x%X\n",
//			callStack.AddrPC.Offset,
//			callStack.AddrReturn.Offset,
//			callStack.AddrFrame.Offset,
//			callStack.AddrStack.Offset,
//			callStack.FuncTableEntry
//			);

		if( index == maxDepth || !bResult ) 
			break;
		if( callStack.AddrFrame.Offset == 0 && index ) 
		{
			if( ++failures > 3 )
				break;
			continue;
		}
		if( (int)index < minDepth)
			continue;

		GetFunctionInfoFromAddresses( callStack.AddrPC.Offset, callStack.AddrFrame.Offset, symInfo, hProcess );
		GetSourceInfoFromAddress( callStack.AddrPC.Offset, srcInfo, hProcess );

		OutputDebugStringFormat( _T("     %s : %s\n"), srcInfo, symInfo );
	}
}

void StackTrace( HANDLE hThread, LPCTSTR lpszMessage, HANDLE hProcess )
{
#ifdef ASSUME_SINGLE_HPROCESS
	hProcess = s_hProcess;
#endif
	StackTraceOfDepth(hThread, lpszMessage, 0, -1, hProcess);
}


void FunctionParameterInfo(HANDLE hThread, HANDLE hProcess)
{
#ifdef ASSUME_SINGLE_HPROCESS
	hProcess = s_hProcess;
#endif
	STACKFRAME     callStack;
	BOOL           bResult = FALSE;
	CONTEXT        context;
	TCHAR          lpszFnInfo[BUFFERSIZE];

	::ZeroMemory( &context, sizeof(context) );
	context.ContextFlags = CONTEXT_FULL;

	if ( !GetThreadContext( hThread, &context ) )
	{
	   OutputDebugStringFormat( _T("Function info(thread=0x%X) failed.\n") );
		return;
	}
	
	::ZeroMemory( &callStack, sizeof(callStack) );
	callStack.AddrPC.Offset    = context.Eip;
	callStack.AddrStack.Offset = context.Esp;
	callStack.AddrFrame.Offset = context.Ebp;
	callStack.AddrPC.Mode      = AddrModeFlat;
	callStack.AddrStack.Mode   = AddrModeFlat;
	callStack.AddrFrame.Mode   = AddrModeFlat;

	for( ULONG index = 0; index < 2; index++ ) 
	{
		bResult = pStackWalk(
			IMAGE_FILE_MACHINE_I386,
			hProcess,
			hThread,
			&callStack,
			&context, 
			NULL,
			pSymFunctionTableAccess,
			pSymGetModuleBase,
			NULL);
	}

	if ( bResult && callStack.AddrFrame.Offset != 0) 
	{
	   GetFunctionInfoFromAddresses( callStack.AddrPC.Offset, callStack.AddrFrame.Offset, lpszFnInfo, hProcess );
	   OutputDebugStringFormat( _T("Function info(thread=0x%X) : %s\n"), hThread, lpszFnInfo );
	}
	else
	   OutputDebugStringFormat( _T("Function info(thread=0x%X) failed.\n") );
}

#endif //_DEBUG && WIN32

#else
#pragma message(__FILE__": (skipped compilation)")
#endif

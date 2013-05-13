/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(FILEHOOKS_INCL) && !defined(UNITY_BUILD)
#define FILEHOOKS_INCL

#include "../global.h"
#include "../../shared/ipc.h"

//HOOKFUNC HANDLE WINAPI MyCreateFileA(
//	LPCSTR lpFileName,
//	DWORD dwDesiredAccess,
//	DWORD dwShareMode,
//	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
//	DWORD dwCreationDisposition,
//	DWORD dwFlagsAndAttributes,
//	HANDLE hTemplateFile
//)
//{
//	debuglog(LCF_FILEIO|LCF_UNTESTED, __FUNCTION__ "(0x%X) called: %s\n", dwDesiredAccess, lpFileName);
//	HANDLE rv = CreateFileA(
//		lpFileName,
//		dwDesiredAccess,
//		dwShareMode,
//		lpSecurityAttributes,
//		dwCreationDisposition,
//		dwFlagsAndAttributes,
//		hTemplateFile
//	);
//	return rv;
//}

HOOKFUNC HANDLE WINAPI MyCreateFileW(
	LPCWSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile
)
{
	debuglog(LCF_FILEIO|LCF_TODO, __FUNCTION__ "(0x%X) called: %S\n", dwDesiredAccess, lpFileName);
	HANDLE rv = CreateFileW(
		lpFileName,
		dwDesiredAccess,
		dwShareMode,
		lpSecurityAttributes,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		hTemplateFile
	);
	return rv;
}

void ApplyFileIntercepts()
{
	static const InterceptDescriptor intercepts [] = 
	{
//	MAKE_INTERCEPT(1, KERNEL32, CreateFileA),
	MAKE_INTERCEPT(1, KERNEL32, CreateFileW),
	};
	ApplyInterceptTable(intercepts, ARRAYSIZE(intercepts));
}

#else
#pragma message(__FILE__": (skipped compilation)")
#endif

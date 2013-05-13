/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef FILETRAMPS_H_INCL
#define FILETRAMPS_H_INCL

//#define CreateFileA TrampCreateFileA
//TRAMPFUNC HANDLE WINAPI CreateFileA(
//	LPCSTR lpFileName,
//	DWORD dwDesiredAccess,
//	DWORD dwShareMode,
//	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
//	DWORD dwCreationDisposition,
//	DWORD dwFlagsAndAttributes,
//	HANDLE hTemplateFile
//) TRAMPOLINE_DEF
#define CreateFileW TrampCreateFileW
TRAMPFUNC HANDLE WINAPI CreateFileW(
	LPCWSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile
) TRAMPOLINE_DEF

#endif

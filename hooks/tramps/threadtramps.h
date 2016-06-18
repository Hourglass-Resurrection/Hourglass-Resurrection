/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#pragma once

#define CreateThread TrampCreateThread
TRAMPFUNC HANDLE WINAPI CreateThread(
		LPSECURITY_ATTRIBUTES lpThreadAttributes,
		SIZE_T dwStackSize,
		LPTHREAD_START_ROUTINE lpStartAddress,
		LPVOID lpParameter,
		DWORD dwCreationFlags,
		LPDWORD lpThreadId
	) TRAMPOLINE_DEF
#define ExitThread TrampExitThread
TRAMPFUNC VOID WINAPI ExitThread(DWORD dwExitCode) TRAMPOLINE_DEF_VOID
#define TerminateThread TrampTerminateThread
TRAMPFUNC BOOL WINAPI TerminateThread(HANDLE hThread, DWORD dwExitCode) TRAMPOLINE_DEF
#define GetExitCodeThread TrampGetExitCodeThread
TRAMPFUNC BOOL WINAPI GetExitCodeThread(HANDLE hThread, LPDWORD lpExitCode) TRAMPOLINE_DEF
#define NtSetInformationThread TrampNtSetInformationThread
TRAMPFUNC NTSTATUS NTAPI NtSetInformationThread(HANDLE ThreadHandle, DWORD ThreadInformationClass, PVOID ThreadInformation, ULONG ThreadInformationLength) TRAMPOLINE_DEF

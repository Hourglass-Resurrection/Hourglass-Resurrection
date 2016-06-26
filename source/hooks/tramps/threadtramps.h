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
	);
#define ExitThread TrampExitThread
TRAMPFUNC VOID WINAPI ExitThread(DWORD dwExitCode);
#define TerminateThread TrampTerminateThread
TRAMPFUNC BOOL WINAPI TerminateThread(HANDLE hThread, DWORD dwExitCode);
#define GetExitCodeThread TrampGetExitCodeThread
TRAMPFUNC BOOL WINAPI GetExitCodeThread(HANDLE hThread, LPDWORD lpExitCode);
#define NtSetInformationThread TrampNtSetInformationThread
TRAMPFUNC NTSTATUS NTAPI NtSetInformationThread(HANDLE ThreadHandle, DWORD ThreadInformationClass, PVOID ThreadInformation, ULONG ThreadInformationLength);

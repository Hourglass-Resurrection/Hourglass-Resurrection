#include <windows.h>

#include "InjectDLL.h"
#include "inject/IATModifier.h"

#include "logging.h"
#include "CustomDLGs.h"

#include "strsafe.h"

extern bool terminateRequest;

bool InjectDLLIntoIDT(DWORD dwInjectProcessID, HANDLE hInjectProcess, HANDLE hInjectThread, LPCWSTR dllPath, bool runFirst)
{
	try
	{
		Process process(dwInjectProcessID, hInjectProcess);
		IATModifier iatModifier(process);
		iatModifier.setImageBase(process.getImageBase(hInjectThread));
		iatModifier.writeIAT(dllPath, runFirst);
		process.clearDebuggerFlag(hInjectThread); // fix for Exit Fate bootup
	}
	catch(const std::runtime_error &e)
	{
		debugprintf(L"Failed to inject DLL \"%s\" into process id 0x%X IAT: %S\n", dllPath, dwInjectProcessID, e.what());
		return false;
	}
	return true;
}

struct InjectDLLThreadFuncInfo
{
	HANDLE hInjectProcess;
	DWORD dwInjectProcessID;
	HANDLE hInjectThread;
	DWORD dwInjectThreadID;
	LPWSTR injectDllPath;
	BOOL injectIsAsyncReady;
	BOOL injectAllowedToFinish;
	bool runDllLast;
};

DWORD WINAPI InjectDLLThreadFunc(LPVOID lpParam) 
{
	InjectDLLThreadFuncInfo* info = (InjectDLLThreadFuncInfo*)lpParam;
	HANDLE hProcess = info->hInjectProcess;
	DWORD dwInjectProcessID = info->dwInjectProcessID;
	HANDLE hInjectThread = info->hInjectThread;
	DWORD dwInjectThreadID = info->dwInjectThreadID;
	LPWSTR dllPath = info->injectDllPath;
	bool runDllLast = info->runDllLast;
	DWORD dwThread = 0;
	size_t dwPathLength = 1024;
	StringCchLengthW(dllPath, dwPathLength, &dwPathLength);

	dwPathLength *= sizeof(WCHAR);

	if (hProcess == NULL)
	{
		PrintLastError(L"CreateProcess", GetLastError());
		terminateRequest = true;
	}


	// modify the IDT (import directory table) (though the IAT is more commonly referred to)
	// so that Windows thinks the game needs to load our DLL first
	bool injected = InjectDLLIntoIDT(dwInjectProcessID, hProcess, hInjectThread, dllPath, !runDllLast);

	if(injected)
	{
		debugprintf(L"Injecting \"%S\" by IAT (method %d) apparently succeeded.\n", dllPath, runDllLast?2:1);
		info->injectIsAsyncReady = TRUE;
		goto done;
	} 

	DWORD exitCode = 0;

	if(exitCode == 0)
	{
		terminateRequest = true;
		if(hProcess != NULL)
		{
			debugprintf(L"Injection failed...\n");
			CustomMessageBox(L"Injection failed...\nYou can (hopefully) find more information in the debug log .txt file.", L"Error", MB_OK | MB_ICONERROR);
		}
		else
		{
			CustomMessageBox(L"The game could not be launched...\nYou can (hopefully) find more information in the debug log .txt file.", L"Error", MB_OK | MB_ICONERROR);
		}
	}
	else // Why is this here??
	{
		debugprintf(L"Injection probably succeeded (0x%X)...\n", exitCode);
	}

done:
	while(!info->injectAllowedToFinish)
	{
		Sleep(5);
	}
	delete info;

	return 0;
}

void InjectDll(HANDLE hProcess, DWORD dwProcessID, HANDLE hThread, DWORD dwThreadID, LPWSTR dllPath, bool runDllLast)
{
	InjectDLLThreadFuncInfo* info = new InjectDLLThreadFuncInfo;
	info->hInjectProcess = hProcess;
	info->dwInjectProcessID = dwProcessID;
	info->hInjectThread = hThread;
	info->dwInjectThreadID = dwThreadID;
	info->injectDllPath = dllPath;
	info->injectIsAsyncReady = FALSE;
	info->injectAllowedToFinish = FALSE;
	info->runDllLast = runDllLast;

	CreateThread(NULL, 0, InjectDLLThreadFunc, info, 0, NULL);

	while(!info->injectIsAsyncReady)
	{
		Sleep(5);
	}
	info->injectAllowedToFinish = TRUE;
}

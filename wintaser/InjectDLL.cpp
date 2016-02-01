#include <windows.h>

#include "InjectDLL.h"
#include "inject/IATModifier.h"

#include "logging.h"
#include "CustomDLGs.h"

#include "strsafe.h"

extern bool terminateRequest;

bool InjectDLLIntoIDT(DWORD dwInjectProcessID, HANDLE hInjectProcess, HANDLE hInjectThread, const char* dllPath, bool runFirst)
{
	try
	{
		Process process(dwInjectProcessID, hInjectProcess);
		IATModifier iatModifier(process);
		iatModifier.setImageBase(process.getImageBase(hInjectThread));
		iatModifier.writeIAT(dllPath, runFirst);
		process.clearDebuggerFlag(hInjectThread); // fix for Exit Fate bootup
	}
	catch(std::runtime_error e)
	{
		debugprintf("Failed to inject DLL \"%s\" into process id 0x%X IAT: %s\n", dllPath, dwInjectProcessID, e.what());
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
	TCHAR* injectDllPath;
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
	TCHAR* dllPath = info->injectDllPath;
	bool runDllLast = info->runDllLast;
	DWORD dwThread = 0;
	size_t dwPathLength = 1024;
	StringCchLength(dllPath, dwPathLength, &dwPathLength);

	dwPathLength *= sizeof(WCHAR);

	if (hProcess == NULL)
	{
		PrintLastError("CreateProcess", GetLastError());
		terminateRequest = true;
	}


	// modify the IDT (import directory table) (though the IAT is more commonly referred to)
	// so that Windows thinks the game needs to load our DLL first
	bool injected = InjectDLLIntoIDT(dwInjectProcessID, hProcess, hInjectThread, dllPath, !runDllLast);

	if(injected)
	{
		debugprintf("Injecting \"%s\" by IAT (method %d) apparently succeeded.\n", dllPath, runDllLast?2:1);
		info->injectIsAsyncReady = TRUE;
		goto done;
	} 

	DWORD exitCode = 0;

	if(exitCode == 0)
	{
		terminateRequest = true;
		if(hProcess != NULL)
		{
			debugprintf("Injection failed...\n");
			CustomMessageBox("Injection failed...\nYou can (hopefully) find more information in the debug log .txt file.", "Error", MB_OK | MB_ICONERROR);
		}
		else
		{
			CustomMessageBox("The game could not be launched...\nYou can (hopefully) find more information in the debug log .txt file.", "Error", MB_OK | MB_ICONERROR);
		}
	}
	else // Why is this here??
	{
		debugprintf("Injection probably succeeded (0x%X)...\n", exitCode);
	}

done:
	while(!info->injectAllowedToFinish)
	{
		Sleep(5);
	}
	delete info;

	return 0;
}

void InjectDll(HANDLE hProcess, DWORD dwProcessID, HANDLE hThread, DWORD dwThreadID, TCHAR *dllPath, bool runDllLast)
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

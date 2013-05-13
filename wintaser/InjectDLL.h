#pragma once

void InjectDll(HANDLE hProcess, DWORD dwProcessID, HANDLE hThread, DWORD dwThreadID, TCHAR *dllPath, bool runDllLast=false);
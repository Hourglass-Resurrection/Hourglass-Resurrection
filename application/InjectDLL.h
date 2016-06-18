#pragma once

/** Puts hourglass's DLL in the game's address space. , in the DLL
 * hooks are created for every function that needs to be overloaded.
 * FIXME dynamically loaded DLL goes through this. 
 * Games loading with LoadLibrary will just not work for now, 
 *                this is a MAJOR project that requires A LOT of code 
 *                re-structure.
 */
void InjectDll(HANDLE hProcess, DWORD dwProcessID, HANDLE hThread, DWORD dwThreadID, TCHAR *dllPath, bool runDllLast=false);

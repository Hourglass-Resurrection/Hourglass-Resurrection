/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(INTERCEPT_C_INCL) && !defined(UNITY_BUILD)
#define INTERCEPT_C_INCL

#include <windows.h>
#include "print.h"
#include "global.h"
#include "intercept.h"
#include "../shared/asm.h"
#include "../shared/ipc.h"

BOOL InterceptGlobalFunction(FARPROC dwAddressToIntercept, FARPROC dwReplaced, FARPROC dwTrampoline, bool trampolineOnly, BOOL rvOnSkip)
{
	if(!dwAddressToIntercept)
		return FALSE;

	enum { JMP_REL32 = 0xE9 };

	BYTE* pTargetHead = (BYTE*)dwAddressToIntercept;
	BYTE* pTargetTail = pTargetHead;
	BYTE* pTramp = (BYTE*)dwTrampoline;
	BYTE* pHook = (BYTE*)dwReplaced;

	// trampolines work by running a copy of the first few bytes of the target function
	// and then jumping to the remainder of the target function.
	// this totally breaks down if the first few bytes of the target function is a jump.
	// so, the first thing we do is detect if the target starts with a jump,
	// and if so, we follow where the jump goes instead of hooking the target directly.
	if(tasflags.movieVersion >= 65)
	{
		for(int i = 0; *pTargetTail == JMP_REL32; i++)
		{
			int diff = *(DWORD*)(pTargetTail+1) + 5;
			pTargetTail += diff;
			if(pTargetTail == pHook)
			{
				if(i == 0)
					debugprintf("already hooked. skipping.\n");
				else
					debugprintf("already hooked (from 0x%X to 0x%X), although the hook chain is longer than expected. skipping.\n", pTargetTail-diff, pTargetTail);
				return rvOnSkip;
			}
			else
			{
				if(i < 64)
					debugprintf("rejump detected in target (from 0x%X to 0x%X). following chain...\n", pTargetTail-diff, pTargetTail);
				else
				{
					debugprintf("hook chain is too long. target function jumps to itself? skipping.\n");
					return rvOnSkip;
				}
			}
		}

		if(pTargetHead == pHook || pTramp == pHook || pTargetHead == pTramp)
		{
			debugprintf("bad input? target=0x%X, hook=0x%X, tramp=0x%X. skipping hook.\n", pTargetHead, pHook, pTramp);
			return rvOnSkip;
		}
	}
	else // old version. if avast! is installed, this results in an incorrect trampoline which will crash the game.
	{
		if(*pTargetHead == JMP_REL32)
		{
			int diff = *(DWORD*)(pTargetHead+1) + 5;
			pTargetHead += diff;
			debugprintf("rejump detected (from 0x%X to 0x%X). attempting to remove...\n", dwAddressToIntercept, pTargetHead);
			dwAddressToIntercept = (FARPROC)pTargetHead;
		}
		pTargetTail = pTargetHead;
		if(pTargetHead == pHook)
		{
			debugprintf("already hooked (0x%X). skipping.\n", pTargetHead);
			return rvOnSkip;
		}
	}

	debuglog(LCF_HOOK, "want to hook 0x%X to call 0x%X, and want trampoline 0x%X to call 0x%X\n", pTargetHead, pHook, pTramp, pTargetTail);

	if(*pTramp == JMP_REL32)
	{
		int diff = *(DWORD*)(pTramp+1) + 5;
		debugprintf("rejump detected in trampoline (from 0x%X to 0x%X). giving up.\n", pTramp, pTramp+diff);
		return rvOnSkip;
	}

	// we'll have to overwrite 5 bytes, which means we have to backup at least 5 bytes (up to next instruction boundary)
	int offset = 0;
	while(offset < 5)
		offset += instructionLength(pTargetTail + offset);

	DWORD dwOldProtect;

	// in the trampoline, write the first 5+ bytes of the target function followed by a jump to our hook
	VirtualProtect((void*)dwTrampoline, 5+offset, PAGE_EXECUTE_READWRITE, &dwOldProtect); 
	for(int i=0; i<offset; i++) 
		*pTramp++ = *pTargetTail++; 
	*pTramp++ = JMP_REL32;
	*((int*)pTramp) = (int)(pTargetTail - (pTramp + 4)); 
	VirtualProtect((void*)dwTrampoline, 5+offset, PAGE_EXECUTE, &dwOldProtect); 

	if(!trampolineOnly)
	{
		// overwrite the first 5 bytes of the target function with a jump to our hook
		VirtualProtect((void*)dwAddressToIntercept, 5, PAGE_EXECUTE_READWRITE, &dwOldProtect); 
		*pTargetHead++ = JMP_REL32;
		*((signed int *)(pTargetHead)) = (int)(pHook - (pTargetHead +4)); 
		VirtualProtect((void*)dwAddressToIntercept, 5, PAGE_EXECUTE, &dwOldProtect); 
	}

	// flush the instruction cache to make sure the modified code is executed
	FlushInstructionCache(GetCurrentProcess(), NULL, NULL); 

	return TRUE; 
}

// replace a virtual function slot of an object
BOOL HookVTable(void* iface, int entry, FARPROC replace, FARPROC& oldfuncPointer, const char* debugname)
{
	size_t* pVTable = (size_t*)*(size_t*)iface;
	FARPROC oldPtr = (FARPROC)pVTable[entry];
	if(oldPtr == replace)
		return FALSE; // already hooked, re-hooking would be disastrous
	if(!oldPtr)
		return FALSE; // uh, I guess this would be bad too
	DWORD dwOldProt;
	VirtualProtect(&pVTable[entry], sizeof(size_t), PAGE_EXECUTE_READWRITE, &dwOldProt);
	oldfuncPointer = oldPtr;
	pVTable[entry] = (size_t)replace;
	VirtualProtect(&pVTable[entry], sizeof(size_t), dwOldProt, &dwOldProt);
	FlushInstructionCache(GetCurrentProcess(), NULL, NULL); 
	debugprintf("HOOKING: %s: %d, 0x%X -> 0x%X\n", debugname, entry, (DWORD)oldfuncPointer, (DWORD)replace);
	return TRUE;
}



#include <string>
#include <map>
#include <vector>
struct InterceptAPIArgs
{
	std::string funcName;
	FARPROC dwReplaced;
	FARPROC dwTrampoline;
	bool trampolineOnly;
	const char* dllName;
//	const char** dllNameArray; // if nonzero, we assume this is a dynamictramp hook and interpret the other arguments differently
};
struct lessicmp
{
   bool operator() (const std::string& a, const std::string& b) const
   {
      return(stricmp(a.c_str(), b.c_str()) < 0);
   }
};
std::map<std::string, std::vector<InterceptAPIArgs>, lessicmp> pendingInterceptAPICalls;
//std::map<std::string, std::vector<InterceptAPIArgs>, lessicmp> delayHookedInterceptAPICalls;
//std::vector<InterceptAPIArgs> multiDllInterceptAPICalls;

//void InterceptAllDllAPI(const char* c_szDllName, const char* c_szApiName, FARPROC dwReplaced, FARPROC dwTrampoline, bool trampolineOnly)
//{
//	InterceptAPIArgs args = {c_szApiName, dwReplaced, dwTrampoline, trampolineOnly, c_szDllName};
//	multiDllInterceptAPICalls.push_back(args);
//}

BOOL InterceptAPI(const char* c_szDllName, const char* c_szApiName,
				  FARPROC dwReplaced, FARPROC dwTrampoline, bool trampolineOnly, bool forceLoad, bool allowTrack, BOOL rvOnSkip)
{
	debuglog(LCF_MODULE|LCF_HOOK, "InterceptAPI(%s.%s) %s\n", c_szDllName, c_szApiName, "started...");

	HMODULE hModule = GetModuleHandle(c_szDllName);
	if(!hModule && forceLoad)
	{
		debugprintf("WARNING: Loading module %s to hook %s\n", c_szDllName, c_szApiName);
		// dangerous according to the documentation,
		// but the alternative is usually to fail spectacularly anyway
		// and it seems to work for me...
		hModule = LoadLibrary(c_szDllName);
	}
	FARPROC dwAddressToIntercept = GetProcAddress(hModule, (char*)c_szApiName); 
	BOOL rv = InterceptGlobalFunction(dwAddressToIntercept, dwReplaced, dwTrampoline, trampolineOnly, rvOnSkip);
	if(rv || allowTrack)
		debugprintf("InterceptAPI(%s.%s) %s \t(0x%X, 0x%X)\n", c_szDllName, c_szApiName, rv ? (trampolineOnly ? "bypassed." : "succeeded.") : "failed!", dwAddressToIntercept, dwTrampoline);

	if(/*!rv && */allowTrack)
	{
		InterceptAPIArgs args = {c_szApiName, dwReplaced, dwTrampoline, trampolineOnly, c_szDllName};
		pendingInterceptAPICalls[c_szDllName].push_back(args);
	}

	return rv;
}

static void HookDelayLoadedDll(const char* c_szDllName, std::vector<InterceptAPIArgs>& vec)
{
	for(int i = (int)vec.size()-1; i >= 0; i--)
	{
		InterceptAPIArgs& args = vec[i];
		BOOL success;
//		if(!args.dllNameArray)
			success = InterceptAPI(c_szDllName, args.funcName.c_str(), args.dwReplaced, args.dwTrampoline, args.trampolineOnly, false, /*true*/false, TRUE);
//		else
//			success = InterceptAPIDynamicTramp(c_szDllName, args.funcName.c_str(), (FARPROC*&)*(FARPROC**)args.dwReplaced, (FARPROC*&)*(FARPROC**)args.dwTrampoline, args.trampolineOnly, args.dllNameArray);
		//if(success)
		//	delayHookedInterceptAPICalls[c_szDllName].push_back(args);
		//vec.erase(vec.begin() + i); // even on failure, because InterceptAPI re-adds it in that case
	}
	////debugprintf("RetryInterceptAPIs: failvec=%d\n", vec.size());
	//if(vec.empty())
	//	pendingInterceptAPICalls.erase(c_szDllName);
}

//static void HookDelayLoadedDll_MultiDll(const char* c_szDllName, std::vector<InterceptAPIArgs>& vec)
//{
//	for(int i = (int)vec.size()-1; i >= 0; i--)
//	{
//		InterceptAPIArgs& args = vec[i];
//		const char** nameArray = (const char**)args.dllName;
//		FARPROC* hookArray = (FARPROC*)args.dwReplaced;
//		FARPROC* trampArray = (FARPROC*)args.dwTrampoline;
//		if(*hookArray && *trampArray)
//		{
//			BOOL success = InterceptAPI(c_szDllName, args.funcName.c_str(), *hookArray, *trampArray, args.trampolineOnly, false, false, FALSE);
//			if(success)
//			{
//				args.dwReplaced = (FARPROC)(((FARPROC*)args.dwReplaced) + 1);
//				args.dwTrampoline = (FARPROC)(((FARPROC*)args.dwTrampoline) + 1);
//				if(nameArray && *nameArray)
//				{
//					*nameArray = _strdup(c_szDllName); // I normally never use strdup...
//					args.dllName = (const char*)(((const char**)args.dllName) + 1);
//				}
//			}
//		}
//		else
//		{
//			debuglog(LCF_HOOK|LCF_ERROR, "Unable to hook \"%s\" in \"%s\" because too many other DLLs used it already.\n", args.funcName.c_str(), c_szDllName);
//		}
//	}
//}

//BOOL InterceptAPIDynamicTramp(const char* c_szDllName, const char* c_szApiName, FARPROC*& hookArray, FARPROC*& trampArray, bool trampolineOnly, const char**& nameArray)
//{
//	debuglog(LCF_MODULE, "InterceptAPIDynamicTramp(%s.%s) %s\n", c_szDllName, c_szApiName, "started...");
//
//	if(*hookArray && *trampArray)
//	{
//		BOOL success = InterceptAPI(c_szDllName, c_szApiName, *hookArray, *trampArray, trampolineOnly, false, false, FALSE);
//		debugprintf("InterceptAPIDynamicTramp(%s.%s) %s \t(0x%X, 0x%X)\n", c_szDllName, c_szApiName, success ? (trampolineOnly ? "bypassed." : "succeeded.") : "failed!", *hookArray, *trampArray);
//		if(success)
//		{
//			hookArray++;
//			trampArray++;
//			if(nameArray && *nameArray) // set the dll name slot if present (for debugging/logging purposes)
//			{
//				*nameArray = _strdup(c_szDllName); // I normally never use strdup, but it seems ok here since this can only get called a small number of times and there is no place to deallocate it
//				nameArray++;
//			}
//			return TRUE;
//		}
//		else
//		{
//			InterceptAPIArgs args = {c_szApiName, (FARPROC)&hookArray, (FARPROC)&trampArray, trampolineOnly, c_szDllName, nameArray};
//			pendingInterceptAPICalls[c_szDllName].push_back(args);
//		}
//	}
//	else
//	{
//		debuglog(LCF_HOOK|LCF_ERROR, "Unable to hook \"%s\" in \"%s\" because too many other DLLs used it already.\n", c_szApiName, c_szDllName);
//	}
//	return FALSE;
//}


void RetryInterceptAPIs(const char* c_szDllName)
{
	std::map<std::string, std::vector<InterceptAPIArgs>, lessicmp >::iterator found;

	//for(found = pendingInterceptAPICalls.begin(); found != pendingInterceptAPICalls.end(); found++)
	//{
	//	const char* dllnamecheck = found->first.c_str();
	//	debugprintf("check: \"%s\" vs \"%s\"\n", c_szDllName, dllnamecheck);
	//}

	found = pendingInterceptAPICalls.find(c_szDllName);
	if(found != pendingInterceptAPICalls.end() && !found->second.empty())
	{
		debugprintf("Hooking delayed load DLL: %s\n", c_szDllName);
		HookDelayLoadedDll(c_szDllName, found->second);
	}
	//if(!multiDllInterceptAPICalls.empty())
	//{
	//	verbosedebugprintf("Maybe hooking delayed load DLL: %s\n", c_szDllName);
	//	HookDelayLoadedDll_MultiDll(c_szDllName, multiDllInterceptAPICalls);
	//}
}

void UnInterceptUnloadingAPIs(const char* c_szDllName)
{
	debuglog(LCF_MODULE, "Unloaded delayed load DLL: %s\n", c_szDllName);
	//std::map<std::string, std::vector<InterceptAPIArgs>, lessicmp >::iterator found;
	//found = delayHookedInterceptAPICalls.find(c_szDllName);
	//if(found == delayHookedInterceptAPICalls.end())
	//	return;
	//std::vector<InterceptAPIArgs>& vec = found->second;
	//std::vector<InterceptAPIArgs>& vecTo = pendingInterceptAPICalls[c_szDllName];
	//for(int i = (int)vec.size()-1; i >= 0; i--)
	//{
	//	vecTo.push_back(vec[i]);
	//	vec.erase(vec.begin()+i);
	//}
	////debugprintf("UnInterceptUnloadingAPIs: failvec=%d\n", vecTo.size());
	//if(vec.empty())
	//	delayHookedInterceptAPICalls.erase(c_szDllName);
}


void ApplyInterceptTable(const InterceptDescriptor* intercepts, int count)
{
	for(int i=0;i<count;i++)
	{
		const InterceptDescriptor& intercept = intercepts[i];
		//if(!intercept.functionName)
		//	break;
		//if(intercept.enabled == 3 || intercept.enabled == -3) // MAKE_INTERCEPT_DYNAMICTRAMP
		//{
		//	InterceptAPIDynamicTramp(intercept.dllName, intercept.functionName, (FARPROC*&)intercept.replacementFunction, (FARPROC*&)intercept.trampolineFunction, intercept.enabled <= 0, intercept.dllNameArray);
		//}
		//else if(intercept.enabled == 4 || intercept.enabled == -4) // MAKE_INTERCEPT_ALLDLLS
		//{
		//	InterceptAllDllAPI((const char*)intercept.dllNameArray, intercept.functionName, intercept.replacementFunction, intercept.trampolineFunction, intercept.enabled <= 0);
		//}
		//else // MAKE_INTERCEPT or MAKE_INTERCEPT2 or MAKE_INTERCEPT3
		{
			InterceptAPI(intercept.dllName, intercept.functionName, intercept.replacementFunction, intercept.trampolineFunction, intercept.enabled <= 0, intercept.enabled != 0 && intercept.enabled != 1, true);
		}
	}
}

#else
#pragma message(__FILE__": (skipped compilation)")
#endif

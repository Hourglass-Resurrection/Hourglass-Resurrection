/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(INJECTPROCESS_C_INCL) && !defined(UNITY_BUILD)
#define INJECTPROCESS_C_INCL

// (this file came from N-InjectLib, which the author released to the public domain)
// modified to get rid of stuff I don't need

#include "process.h"
#include <iostream>
#include <sstream>

using namespace std;

Process::Process(DWORD processID, HANDLE hProcess) :
	hProcess_(hProcess),
	processID_(processID),
	pebAddr_(0)
{
	// disabled because some scanners might consider any calls to OpenProcess to be suspicious
	// even though it's simply an official win32 api function.
	// we don't need to call it because we know we created the process ourselves,
	// and the hProcess that CreateProcess gives us has full access rights already granted.
	//if(!hProcess_)
	//	hProcess_ = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, processID);
	if (hProcess_ == NULL)
	{
		DWORD lastErr = GetLastError();
		stringstream ss;
		ss << "Failed to get appropriate process access rights for process id: " << processID
		   << ", system error code: " << lastErr;
		throw ProcessHandleException(ss.str());
	}
}

void Process::writeMemory(LPVOID address, LPCVOID data, DWORD size) const
{
	SIZE_T written = 0;
	WriteProcessMemory(hProcess_, address, data, size, &written);
	if (written != size) throw MemoryAccessException("Write memory failed!");
}

void Process::readMemory(LPVOID address, LPVOID buffer, DWORD size) const
{
	SIZE_T read = 0;
	ReadProcessMemory(hProcess_, address, buffer, size, &read);
	if (read != size) throw MemoryAccessException("Read memory failed!");
}

MEMORY_BASIC_INFORMATION Process::queryMemory(LPVOID address) const
{
	MEMORY_BASIC_INFORMATION mbi;
	SIZE_T retVal = VirtualQueryEx(hProcess_, address, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
	if (retVal == 0) throw MemoryQueryException("Unable to query memory");
	return mbi;
}

DWORD Process::protectMemory(LPVOID address, SIZE_T size, DWORD protect) const
{
	DWORD oldProtect;
	BOOL retVal = VirtualProtectEx(hProcess_, address, size, protect, &oldProtect);
	if (retVal == FALSE) throw MemoryProtectException("Unable to set memory protection", address);
	return oldProtect;
}

LPVOID Process::allocMem(DWORD size) const
{
	return allocMem(size, MEM_RESERVE | MEM_COMMIT);	
}

LPVOID Process::allocMem(DWORD size, DWORD allocationType) const
{
	return allocMem(size, NULL, allocationType);
}

LPVOID Process::allocMem(DWORD size, LPVOID desiredAddress, DWORD allocationType) const
{
	LPVOID addr = VirtualAllocEx(hProcess_, desiredAddress, size, allocationType, PAGE_EXECUTE_READWRITE);
	if (addr == NULL) throw MemoryAllocationException("Failed to allocate memory");
	return addr;
}

bool Process::freeMem(LPVOID address) const
{
	return (VirtualFreeEx(hProcess_, address, 0, MEM_RELEASE) != 0);
}


void Process::throwSysError(const char* msg, DWORD lastError) const
{
	std::ostringstream oss;
	oss << msg << ", system error was: " << lastError;
	throw std::runtime_error(oss.str());
}

// also works if process is suspended and not fully initialized yet
uintptr_t Process::getImageBase(HANDLE hThread) const
{
	CONTEXT context;
	context.ContextFlags = CONTEXT_SEGMENTS;
	if (!GetThreadContext(hThread, &context))
	{
		throwSysError("Error while retrieving thread context to determine IBA",  GetLastError());
	}

	// translate FS selector to virtual address
	LDT_ENTRY ldtEntry;
	if (!GetThreadSelectorEntry(hThread, context.SegFs, &ldtEntry))
	{
		throwSysError("Error while translating FS selector to virtual address",  GetLastError());
	}

	uintptr_t fsVA = (ldtEntry.HighWord.Bytes.BaseHi) << 24
		| (ldtEntry.HighWord.Bytes.BaseMid) << 16 | (ldtEntry.BaseLow);

	uintptr_t iba = 0;
	SIZE_T read;
	// finally read image based address from PEB:[8]
	if (!(ReadProcessMemory(hProcess_, (LPCVOID)(fsVA+0x30), &pebAddr_, sizeof(uintptr_t), &read)
		&& ReadProcessMemory(hProcess_, (LPCVOID)(pebAddr_+8), &iba, sizeof(uintptr_t), &read)))
	{
		throwSysError("Error while reading process memory to retrieve image base address", GetLastError());
	}
	return iba;
}

void Process::clearDebuggerFlag(HANDLE hThread)
{
	if(!pebAddr_)
		getImageBase(hThread);
	if(!pebAddr_)
		return;
	SIZE_T read;
	uintptr_t base = 0;
	bool debugged = 0;
	if(ReadProcessMemory(hProcess_, (LPCVOID)(pebAddr_+2), &debugged, sizeof(bool), &read)
	&& debugged)
	{
		debugged = 0;
		SIZE_T written;
		WriteProcessMemory(hProcess_, (LPVOID)(pebAddr_+2), &debugged, sizeof(bool), &written);
	}
}

#else
#pragma message(__FILE__": (skipped compilation)")
#endif

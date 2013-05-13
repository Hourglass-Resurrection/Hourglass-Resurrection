/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

// (this file came from N-InjectLib, which the author released to the public domain)
// modified to get rid of stuff I don't need

#pragma once

#include <iostream>
#include <vector>
#include <Windows.h>
#include <TlHelp32.h>

class Process
{
public:

	Process(DWORD processID, HANDLE hProcess);
	//Process::Process(const Process& instance);
	//Process& Process::operator=(const Process& instance);
	//~Process();
	
	LPVOID allocMem(DWORD size) const;
	LPVOID allocMem(DWORD size, DWORD allocationType) const;
	LPVOID allocMem(DWORD size, LPVOID desiredAddress, DWORD allocationType) const;
	bool freeMem(LPVOID address) const;
	void writeMemory(LPVOID address, LPCVOID data, DWORD size) const;
	void readMemory(LPVOID address, LPVOID buffer, DWORD size) const;
	MEMORY_BASIC_INFORMATION queryMemory(LPVOID address) const;
	DWORD protectMemory(LPVOID address, SIZE_T size, DWORD protect) const;
	
	uintptr_t getImageBase(HANDLE hThread) const;

	void clearDebuggerFlag(HANDLE hThread);

	HANDLE hProcess_;
private:

	bool duplicateHandle(HANDLE hSrc, HANDLE* hDest);
	void throwSysError(const char* msg, DWORD lastError) const;

	DWORD processID_;
	mutable uintptr_t pebAddr_; // cache peb address
};

// handle error
class ProcessHandleException : public std::runtime_error
{
public:
	ProcessHandleException::ProcessHandleException(const std::string& msg) : std::runtime_error(msg) {};
};

// anything with memory
class ProcessMemoryException : public std::runtime_error
{
public:
	ProcessMemoryException::ProcessMemoryException(const std::string& msg, LPVOID address) : std::runtime_error(msg), address_(address) {};
	LPVOID getAddress() { return address_; };
private:
	LPVOID address_;
};

// access memory
class MemoryAccessException : public std::runtime_error
{
public:
	MemoryAccessException::MemoryAccessException(const std::string& msg) : std::runtime_error(msg) {};	
};

// allocate
class MemoryAllocationException : public std::runtime_error
{
public:
	MemoryAllocationException::MemoryAllocationException(const std::string& msg) : std::runtime_error(msg) {};	
};

// query memory
class MemoryQueryException : public std::runtime_error
{
public:
	MemoryQueryException::MemoryQueryException(const std::string& msg) : std::runtime_error(msg) {};	
};

// protect memory
class MemoryProtectException : public ProcessMemoryException
{
public:
	MemoryProtectException::MemoryProtectException(const std::string& msg, LPVOID address) : ProcessMemoryException(msg, address) {};	
};
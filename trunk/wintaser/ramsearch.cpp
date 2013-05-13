/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#if !defined(RAMSEARCH_C_INCL) && !defined(UNITY_BUILD)
#define RAMSEARCH_C_INCL

// A few notes about this implementation of a RAM search window:
// (although, note first that it was created for a different application.
//  the tradeoffs it makes were focused towards a 16-bit console emulator
//  and not might be particularly appropriate here. consider using MHS instead.)
//
// Speed of update was one of the highest priories.
// This is because I wanted the RAM search window to be able to
// update every single value in RAM every single frame, and
// keep track of the exact number of frames across which each value has changed,
// without causing the emulation to run noticeably slower than normal.
//
// The data representation was changed from one entry per valid address
// to one entry per contiguous range of uneliminated addresses
// which references uniform pools of per-address properties.
// - This saves time when there are many items because
//   it minimizes the amount of data that needs to be stored and processed per address.
// - It also saves time when there are few items because
//   it ensures that no time is wasted in iterating through
//   addresses that have already been eliminated from the search.
//
// The worst-case scenario is when every other item has been
// eliminated from the search, maximizing the number of regions.
// This implementation manages to handle even that pathological case
// acceptably well. In fact, it still updates faster than the previous implementation.
// The time spent setting up or clearing such a large number of regions
// is somewhat horrendous, but it seems reasonable to have poor worst-case speed
// during these sporadic "setup" steps to achieve an all-around faster per-update speed.
// (You can test this case by performing the search: Modulo 2 Is Specific Address 0)

// this ram search module was made for Gens (Sega Genesis/CD/32X games)
// and adapted for use in winTASer (PC games)
// which means the amount of memory it has to deal with has greatly increased.
// it also has to use a slower call to get at the memory because it's owned by another process.
// as a result, it's now kind of slow instead of blazingly fast.
// it's still good enough for me in certain games (Cave Story), though.
//
// since dynamically allocated memory was not a possibility on the Genesis,
// dynamically allocated regions of memory are only added when "Reset" is clicked.
// also, only a small part of the application's total memory is searched.
// because of the way it is currently written, this module needs to allocate
// 8 times the amount of memory it is able to search.
//
// possibly the only advantage this has over other PC memory search tools
// is that the search results are exactly synchronized with the frame boundaries,
// which makes it easy to narrow down results using the "change count" number.

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#define _WIN32_WINNT 0x0500
#include <windows.h>
#include "resource.h"
#include "ramsearch.h"
#include "ramwatch.h"
#include "../shared/winutil.h"
#include <assert.h>
#include <commctrl.h>
#include <list>
#include <vector>
#include <math.h>
#ifdef _WIN32
   #include "BaseTsd.h"
   typedef INT_PTR intptr_t;
#else
   #include "stdint.h"
#endif

#if defined(_MSC_VER) && (_MSC_VER <= 1310)

#define LINELINELINELINE(x) #x
#define LINELINELINE(x) LINELINELINELINE(x)
#define LINELINE LINELINELINE(__LINE__)
#pragma message("ramsearch.inl(" LINELINE ") : warning: your compiler is too old to compile this file. (VS2005 or newer has __VA_ARGS__ support)")

// dummy implementation for old compiler, enough to get the file compiling but doing basically nothing
// (besides the minimum required for ramwatch.cpp to work)
void ResetResults() {}
void CloseRamWindows() {}
void ReopenRamWindows() {}
void Update_RAM_Search()
{
	extern HWND RamWatchHWnd;
	if(RamWatchHWnd)
	{
		Update_RAM_Watch();
	}
}
void InitRamSearch() {}
void reset_address_info () {}
extern HANDLE hGameProcess;
void signal_new_frame () {}
bool IsHardwareAddressValid(HWAddressType address)
{
	char temp [4];
	return ReadProcessMemory(hGameProcess, (const void*)address, (void*)temp, 1, NULL)
		&& WriteProcessMemory(hGameProcess, (void*)address, (const void*)temp, 1, NULL);
}
unsigned int ReadValueAtHardwareAddress(HWAddressType address, unsigned int size)
{
	unsigned int value = 0;
	ReadProcessMemory(hGameProcess, (const void*)address, (void*)&value, size, NULL);
	return value;
}
LRESULT CALLBACK RamSearchProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	extern HWND RamSearchHWnd;
	RamSearchHWnd = NULL;
	EndDialog(hDlg, true);
	return true;
}
bool noMisalign=0;
int ResultCount=0;

void DeallocateRamSearch(){}


#else // actual implementation on a capable-enough compiler:


struct MemoryRegion
{
	HWAddressType hardwareAddress; // hardware address of the start of this region
	unsigned int size; // number of bytes to the end of this region

	unsigned int virtualIndex; // index into s_prevValues, s_curValues, and s_numChanges, valid after being initialized in ResetMemoryRegions()
	unsigned int itemIndex; // index into listbox items, valid when s_itemIndicesInvalid is false
};

int MAX_RAM_SIZE = 0;
static unsigned char* s_prevValues = 0; // values at last search or reset
static unsigned char* s_curValues = 0; // values at last frame update
static unsigned short* s_numChanges = 0; // number of changes of the item starting at this virtual index address
static MemoryRegion** s_itemIndexToRegionPointer = 0; // used for random access into the memory list (trading memory size to get speed here, too bad it's so much memory), only valid when s_itemIndicesInvalid is false
static BOOL s_itemIndicesInvalid = true; // if true, the link from listbox items to memory regions (s_itemIndexToRegionPointer) and the link from memory regions to list box items (MemoryRegion::itemIndex) both need to be recalculated
static BOOL s_prevValuesNeedUpdate = true; // if true, the "prev" values should be updated using the "cur" values on the next frame update signaled
static unsigned int s_maxItemIndex = 0; // max currently valid item index, the listbox sometimes tries to update things past the end of the list so we need to know this to ignore those attempts

extern HWND RamSearchHWnd;
extern HWND RamWatchHWnd;
extern HWND hWnd;
extern HANDLE hGameProcess;
extern HINSTANCE hInst;
extern CRITICAL_SECTION g_processMemCS;
static char Str_Tmp_RS [1024];

int disableRamSearchUpdate = false;



//static const MemoryRegion s_prgRegion    = {  0x020000, SEGACD_RAM_PRG_SIZE, (unsigned char*)Ram_Prg,     true};
//static const MemoryRegion s_word1MRegion = {  0x200000, SEGACD_1M_RAM_SIZE,  (unsigned char*)Ram_Word_1M, true};
//static const MemoryRegion s_word2MRegion = {  0x200000, SEGACD_2M_RAM_SIZE,  (unsigned char*)Ram_Word_2M, true};
//static const MemoryRegion s_z80Region    = {  0xA00000, Z80_RAM_SIZE,        (unsigned char*)Ram_Z80,     true};
//static const MemoryRegion s_68kRegion    = {  0xFF0000, _68K_RAM_SIZE,       (unsigned char*)Ram_68k,     true};
//static const MemoryRegion s_32xRegion    = {0x06000000, _32X_RAM_SIZE,       (unsigned char*)_32X_Ram,    false};

// list of contiguous uneliminated memory regions
typedef std::list<MemoryRegion> MemoryList;
static MemoryList s_activeMemoryRegions;
static CRITICAL_SECTION s_activeMemoryRegionsCS;

// for undo support (could be better, but this way was really easy)
static MemoryList s_activeMemoryRegionsBackup;
static int s_undoType = 0; // 0 means can't undo, 1 means can undo, 2 means can redo

void RamSearchSaveUndoStateIfNotTooBig(HWND hDlg);
static const int tooManyRegionsForUndo = 10000;



bool IsInNonCurrentYetTrustedAddressSpace(DWORD address);


void ResetMemoryRegions()
{
//	Clear_Sound_Buffer();
	EnterCriticalSection(&s_activeMemoryRegionsCS);

	s_activeMemoryRegions.clear();

	if(hGameProcess)
	{
		EnterCriticalSection(&g_processMemCS);

		MEMORY_BASIC_INFORMATION mbi = {0};
		SYSTEM_INFO si = {0};
 		GetSystemInfo(&si);
		// walk process addresses
		void* lpMem = si.lpMinimumApplicationAddress;
		int totalSize = 0;
		while (lpMem < si.lpMaximumApplicationAddress)
		{
			VirtualQueryEx(hGameProcess,lpMem,&mbi,sizeof(MEMORY_BASIC_INFORMATION));
			// increment lpMem to next region of memory
			lpMem = (LPVOID)((unsigned char*)mbi.BaseAddress + (DWORD)mbi.RegionSize);

			// check if it's readable and writable
			// (including read-only regions gives us WAY too much memory to search)
			if(((mbi.Protect & PAGE_READWRITE)
			 || (mbi.Protect & PAGE_EXECUTE_READWRITE))
			&& !(mbi.Protect & PAGE_GUARD)
			&& (mbi.State & MEM_COMMIT)
//			&& (mbi.Type & (MEM_PRIVATE | MEM_IMAGE))
			)
			{
				//if((unsigned int)mbi.BaseAddress >= 0x01400000
				//&& (unsigned int)-(signed int)((unsigned int)mbi.BaseAddress|0xF0000000) >= 0x01400000
				//&& mbi.RegionSize > 0x4000)
				//if(mbi.RegionSize > 0x40000)
				//	mbi.RegionSize = 0x40000;
				//if(!((unsigned int)mbi.BaseAddress >= 0x400000 && mbi.BaseAddress < 0x400000 + 0x400000))
				//if((unsigned int)mbi.BaseAddress > 0x00400000 + 0x00800000
				//&& ((unsigned int)mbi.BaseAddress < 0x7ff00000 || (unsigned int)mbi.BaseAddress >= 0x80000000))
				//	continue; // 0x7f000000  ... 0x7ffb0000
				if(!IsInNonCurrentYetTrustedAddressSpace((unsigned int)mbi.BaseAddress))
					continue;
				//BOOL a = (mbi.State & MEM_PRIVATE);
				//BOOL b = (mbi.State & MEM_IMAGE);
				//BOOL c = (mbi.State & MEM_MAPPED);

				if(IsHardwareAddressValid((HWAddressType)mbi.BaseAddress))
				{
					//static const int maxRegionSize = 1024*1024*2;
					//if(mbi.RegionSize > maxRegionSize)
					//	mbi.RegionSize = maxRegionSize;

					MemoryRegion region = { (HWAddressType)mbi.BaseAddress, mbi.RegionSize };
					s_activeMemoryRegions.push_back(region);
				}
			}
		}

		LeaveCriticalSection(&g_processMemCS);
	}




	int nextVirtualIndex = 0;
	for(MemoryList::iterator iter = s_activeMemoryRegions.begin(); iter != s_activeMemoryRegions.end(); ++iter)
	{
		MemoryRegion& region = *iter;
		region.virtualIndex = nextVirtualIndex;
		//assert(((intptr_t)region.softwareAddress & 1) == 0 && "somebody needs to reimplement ReadValueAtSoftwareAddress()");
		nextVirtualIndex = region.virtualIndex + region.size;
	}
	//assert(nextVirtualIndex <= MAX_RAM_SIZE);

	if(nextVirtualIndex > MAX_RAM_SIZE)
	{
		s_prevValues = (unsigned char*)realloc(s_prevValues, sizeof(char)*(nextVirtualIndex+8));
		memset(s_prevValues, 0, sizeof(char)*(nextVirtualIndex+8));

		s_curValues = (unsigned char*)realloc(s_curValues, sizeof(char)*(nextVirtualIndex+8));
		memset(s_curValues, 0, sizeof(char)*(nextVirtualIndex+8));

		s_numChanges = (unsigned short*)realloc(s_numChanges, sizeof(short)*(nextVirtualIndex+8));
		memset(s_numChanges, 0, sizeof(short)*(nextVirtualIndex+8));

		s_itemIndexToRegionPointer = (MemoryRegion**)realloc(s_itemIndexToRegionPointer, sizeof(MemoryRegion*)*(nextVirtualIndex+8));
		memset(s_itemIndexToRegionPointer, 0, sizeof(MemoryRegion*)*(nextVirtualIndex+8));

		MAX_RAM_SIZE = nextVirtualIndex;
	}
	LeaveCriticalSection(&s_activeMemoryRegionsCS);
}

// eliminates a range of hardware addresses from the search results
// returns 2 if it changed the region and moved the iterator to another region
// returns 1 if it changed the region but didn't move the iterator
// returns 0 if it had no effect
// warning: don't call anything that takes an itemIndex in a loop that calls DeactivateRegion...
//   doing so would be tremendously slow because DeactivateRegion invalidates the index cache
int DeactivateRegion(MemoryRegion& region, MemoryList::iterator& iter, HWAddressType hardwareAddress, unsigned int size)
{
	if(hardwareAddress + size <= region.hardwareAddress || hardwareAddress >= region.hardwareAddress + region.size)
	{
		// region is unaffected
		return 0;
	}
	else if(hardwareAddress > region.hardwareAddress && hardwareAddress + size >= region.hardwareAddress + region.size)
	{
		// erase end of region
		region.size = hardwareAddress - region.hardwareAddress;
		return 1;
	}
	else if(hardwareAddress <= region.hardwareAddress && hardwareAddress + size < region.hardwareAddress + region.size)
	{
		// erase start of region
		int eraseSize = (hardwareAddress + size) - region.hardwareAddress;
		region.hardwareAddress += eraseSize;
		region.size -= eraseSize;
		//region.softwareAddress += eraseSize;
		region.virtualIndex += eraseSize;
		return 1;
	}
	else if(hardwareAddress <= region.hardwareAddress && hardwareAddress + size >= region.hardwareAddress + region.size)
	{
		// erase entire region
		iter = s_activeMemoryRegions.erase(iter);
		s_itemIndicesInvalid = TRUE;
		return 2;
	}
	else //if(hardwareAddress > region.hardwareAddress && hardwareAddress + size < region.hardwareAddress + region.size)
	{
		// split region
		int eraseSize = (hardwareAddress + size) - region.hardwareAddress;
		MemoryRegion region2 = {region.hardwareAddress + eraseSize, region.size - eraseSize, /*region.softwareAddress + eraseSize,*/ region.virtualIndex + eraseSize};
		region.size = hardwareAddress - region.hardwareAddress;
		iter = s_activeMemoryRegions.insert(++iter, region2);
		s_itemIndicesInvalid = TRUE;
		return 2;
	}
}

/*
// eliminates a range of hardware addresses from the search results
// this is a simpler but usually slower interface for the above function
void DeactivateRegion(HWAddressType hardwareAddress, unsigned int size)
{
	for(MemoryList::iterator iter = s_activeMemoryRegions.begin(); iter != s_activeMemoryRegions.end(); )
	{
		MemoryRegion& region = *iter;
		if(2 != DeactivateRegion(region, iter, hardwareAddress, size))
			++iter;
	}
}
*/

// warning: can be slow
void CalculateItemIndices(int itemSize)
{
	AutoCritSect cs(&s_activeMemoryRegionsCS);
	unsigned int itemIndex = 0;
	for(MemoryList::iterator iter = s_activeMemoryRegions.begin(); iter != s_activeMemoryRegions.end(); ++iter)
	{
		MemoryRegion& region = *iter;
		region.itemIndex = itemIndex;
		int startSkipSize = ((unsigned int)(itemSize - (unsigned int)region.hardwareAddress)) % itemSize; // FIXME: is this still ok?
		unsigned int start = startSkipSize;
		unsigned int end = region.size;
		for(unsigned int i = start; i < end; i += itemSize)
			s_itemIndexToRegionPointer[itemIndex++] = &region;
	}
	s_maxItemIndex = itemIndex;
	s_itemIndicesInvalid = FALSE;
}

bool RSVal::print(char* output, char sizeTypeID, char typeID)
{
	switch(typeID)
	{
	case 'f': {
		int len = sprintf(output, "%g", (double)*this); // don't use %f, too long
		// now, I want whole numbers to still show a .0 at the end so they look like floats.
		// I'd check LOCALE_SDECIMAL, but sprintf doesn't seem to use the current locale's decimal separator setting anyway.
		bool floaty = false;
		for(int i = 0; i < len; i++)
		{
			if(output[i] == '.' || output[i] == 'e' || output[i] == ',')
			{
				floaty = true;
				break;
			}
		}
		if(!floaty)
			strcpy(output+len, ".0");
	}	break;
	case 's':
		switch(sizeTypeID)
		{	default:
			case 'b': output += sprintf(output, "%d", (char)((int)*this&0xff));
				if((unsigned int)(((int)*this&0xff)-32) < (unsigned int)(127-32))
					sprintf(output, " ('%c')", (char)((int)*this&0xff));
				break;
			case 'w': sprintf(output, "%d", (short)((int)*this&0xffff)); break;
			case 'd': sprintf(output, "%d", (int)*this); break;
			case 'l': sprintf(output, "%I64d", (long long)*this); break;
		}
		break;
	case 'u':
		switch(sizeTypeID)
		{	default:
			case 'b': output += sprintf(output, "%u", (unsigned char)((int)*this&0xff));
				if((unsigned int)(((int)*this&0xff)-32) < (unsigned int)(127-32))
					sprintf(output, " ('%c')", (unsigned char)((int)*this&0xff));
				break;
			case 'w': sprintf(output, "%u", (unsigned short)((int)*this&0xffff)); break;
			case 'd': sprintf(output, "%u", (unsigned long)(int)*this); break;
			case 'l': sprintf(output, "%I64u", (unsigned long long)(long long)*this); break;
		}
		break;
	default:
	case 'h':
		switch(sizeTypeID)
		{	default:
			case 'b': sprintf(output, "%02x", ((int)*this&0xff)); break;
			case 'w': sprintf(output, "%04x", ((int)*this&0xffff)); break;
			case 'd': sprintf(output, "%08x", (int)*this); break;
			case 'l': sprintf(output, "%016I64x", (long long)*this); break;
		}
		break;
	}
	return true;
}

bool RSVal::scan(const char* input, char sizeTypeID, char typeID)
{
	int inputLen = strlen(input)+1;
	inputLen = min(inputLen, 32);
	char* temp = (char*)_alloca(inputLen);
	strncpy(temp, input, inputLen);
	temp[inputLen-1] = 0;
	for(int i = 0; temp[i]; i++)
		if(toupper(temp[i]) == 'O')
			temp[i] = '0';

	bool forceHex = (typeID == 'h');
	bool readFloat = (typeID == 'f');
	bool readLongLong = (sizeTypeID == 'l');
	bool negate = false;

	char* strPtr = temp;
	while(strPtr[0] == '-')
		strPtr++, negate = !negate;
	if(strPtr[0] == '+')
		strPtr++;
	if(strPtr[0] == '0' && tolower(strPtr[1]) == 'x')
		strPtr += 2, forceHex = true;
	if(strPtr[0] == '$')
		strPtr++, forceHex = true;
	if(strPtr[0] == '\'' && strPtr[1] && strPtr[2] == '\'')
	{
		if(readFloat) forceHex = true;
		sprintf(strPtr, forceHex ? "%X" : "%u", (int)strPtr[1]);
	}
	if(!forceHex && !readFloat)
	{
		const char* strSearchPtr = strPtr;
		while(*strSearchPtr)
		{
			int c = tolower(*strSearchPtr++);
			if(c >= 'a' && c <= 'f')
			{
				forceHex = true;
				break;
			}
			if(c == '.')
			{
				readFloat = true;
				break;
			}
		}
	}
	bool ok = false;
	if(readFloat)
	{
		if(!readLongLong)
		{
			float f = 0;
			if(sscanf(strPtr, forceHex ? "%x" : "%f", &f) > 0)
				ok = true;
			if(negate)
				f = -f;
			*this = f;
		}
		else
		{
			double f = 0;
			if(sscanf(strPtr, forceHex ? "%I64x" : "%lf", &f) > 0)
				ok = true;
			if(negate)
				f = -f;
			*this = f;
		}
	}
	else
	{
		if(!readLongLong)
		{
			int i = 0;
			const char* formatString = forceHex ? "%x" : ((typeID=='s') ? "%d" : "%u");
			if(sscanf(strPtr, formatString, &i) > 0)
				ok = true;
			if(negate)
				i = -i;
			*this = i;
		}
		else
		{
			long long i = 0;
			const char* formatString = forceHex ? "%I64x" : ((typeID=='s') ? "%I64d" : "%I64u");
			if(sscanf(strPtr, formatString, &i) > 0)
				ok = true;
			if(negate)
				i = -i;
			*this = i;
		}
	}
	return ok;
}



template<typename stepType, typename compareType>
void UpdateRegionT(const MemoryRegion& region, const MemoryRegion* nextRegionPtr)
{
	//if(GetAsyncKeyState(VK_SHIFT) & 0x8000) // speed hack
	//	return;

	if(s_prevValuesNeedUpdate)
		memcpy(s_prevValues + region.virtualIndex, s_curValues + region.virtualIndex, region.size + sizeof(compareType) - sizeof(stepType));

	unsigned int startSkipSize = ((unsigned int)(sizeof(stepType) - region.hardwareAddress)) % sizeof(stepType);


	//unsigned char* sourceAddr = region.softwareAddress - region.virtualIndex;
	static unsigned char* memoryBuffer = NULL;
	static unsigned int memoryBufferAllocated = 0;
	if(memoryBufferAllocated < region.size+8)
	{
		memoryBufferAllocated = region.size+8;
		memoryBuffer = (unsigned char*)realloc(memoryBuffer, memoryBufferAllocated);
	}
	ReadProcessMemory(hGameProcess, (const void*)region.hardwareAddress, (void*)memoryBuffer, region.size, NULL);
	if(sizeof(compareType) > 1)
		ReadProcessMemory(hGameProcess, (const void*)(region.hardwareAddress+region.size), (void*)(memoryBuffer+region.size), sizeof(compareType)-1, NULL);
	unsigned char* sourceAddr = memoryBuffer - region.virtualIndex;

	unsigned int indexStart = region.virtualIndex + startSkipSize;
	unsigned int indexEnd = region.virtualIndex + region.size;

	if(sizeof(compareType) == 1)
	{
		for(unsigned int i = indexStart; i < indexEnd; i++)
		{
			if(s_curValues[i] != sourceAddr[i]) // if value changed
			{
				s_curValues[i] = sourceAddr[i]; // update value
				//if(s_numChanges[i] != 0xFFFF)
					s_numChanges[i]++; // increase change count
			}
		}
	}
	else // it's more complicated for non-byte sizes because:
	{    // - more than one byte can affect a given change count entry
	     // - when more than one of those bytes changes simultaneously the entry's change count should only increase by 1
	     // - a few of those bytes can be outside the region

		unsigned int endSkipSize = ((unsigned int)(startSkipSize - region.size)) % sizeof(stepType);
		unsigned int lastIndexToRead = indexEnd + endSkipSize + sizeof(compareType) - sizeof(stepType);
		unsigned int lastIndexToCopy = lastIndexToRead;
		if(nextRegionPtr)
		{
			const MemoryRegion& nextRegion = *nextRegionPtr;
			int nextStartSkipSize = ((unsigned int)(sizeof(stepType) - nextRegion.hardwareAddress)) % sizeof(stepType);
			unsigned int nextIndexStart = nextRegion.virtualIndex + nextStartSkipSize;
			if(lastIndexToCopy > nextIndexStart)
				lastIndexToCopy = nextIndexStart;
		}

		unsigned int nextValidChange [sizeof(compareType)];
		for(unsigned int i = 0; i < sizeof(compareType); i++)
			nextValidChange[i] = indexStart + i;

		for(unsigned int i = indexStart, j = 0; i < lastIndexToRead; i++, j++)
		{
			if(s_curValues[i] != sourceAddr[i]) // if value of this byte changed
			{
				if(i < lastIndexToCopy)
					s_curValues[i] = sourceAddr[i]; // update value
				for(int k = 0; k < sizeof(compareType); k++) // loop through the previous entries that contain this byte
				{
					if(i >= indexEnd+k)
						continue;
					int m = (j-k+sizeof(compareType)) & (sizeof(compareType)-1);
					if(nextValidChange[m] <= i) // if we didn't already increase the change count for this entry
					{
						//if(s_numChanges[i-k] != 0xFFFF)
							s_numChanges[i-k]++; // increase the change count for this entry
						nextValidChange[m] = i-k+sizeof(compareType); // and remember not to increase it again
					}
				}
			}
		}
	}
}

template<typename stepType, typename compareType>
void UpdateRegionsT()
{
	for(MemoryList::iterator iter = s_activeMemoryRegions.begin(); iter != s_activeMemoryRegions.end();)
	{
		const MemoryRegion& region = *iter;
		++iter;
		const MemoryRegion* nextRegion = (iter == s_activeMemoryRegions.end()) ? NULL : &*iter;

		UpdateRegionT<stepType, compareType>(region, nextRegion);
	}

	s_prevValuesNeedUpdate = false;
}

template<typename stepType, typename compareType>
int CountRegionItemsT()
{
	AutoCritSect cs(&s_activeMemoryRegionsCS);
	if(sizeof(stepType) == 1)
	{
		if(s_activeMemoryRegions.empty())
			return 0;

		if(s_itemIndicesInvalid)
			CalculateItemIndices(sizeof(stepType));

		MemoryRegion& lastRegion = s_activeMemoryRegions.back();
		return lastRegion.itemIndex + lastRegion.size;
	}
	else // the branch above is faster but won't work if the step size isn't 1
	{
		int total = 0;
		for(MemoryList::iterator iter = s_activeMemoryRegions.begin(); iter != s_activeMemoryRegions.end(); ++iter)
		{
			MemoryRegion& region = *iter;
			int startSkipSize = ((unsigned int)(sizeof(stepType) - region.hardwareAddress)) % sizeof(stepType);
			total += (region.size - startSkipSize + (sizeof(stepType)-1)) / sizeof(stepType);
		}
		return total;
	}
}

// returns information about the item in the form of a "fake" region
// that has the item in it and nothing else
template<typename stepType, typename compareType>
void ItemIndexToVirtualRegion(unsigned int itemIndex, MemoryRegion& virtualRegion)
{
	if(s_itemIndicesInvalid)
		CalculateItemIndices(sizeof(stepType));

	if(itemIndex >= s_maxItemIndex)
	{
		memset(&virtualRegion, 0, sizeof(MemoryRegion));
		return;
	}

	const MemoryRegion* regionPtr = s_itemIndexToRegionPointer[itemIndex];
	const MemoryRegion& region = *regionPtr;

	int bytesWithinRegion = (itemIndex - region.itemIndex) * sizeof(stepType);
	int startSkipSize = ((unsigned int)(sizeof(stepType) - region.hardwareAddress)) % sizeof(stepType);
	bytesWithinRegion += startSkipSize;
	
	virtualRegion.size = sizeof(compareType);
	virtualRegion.hardwareAddress = region.hardwareAddress + bytesWithinRegion;
	//virtualRegion.softwareAddress = region.softwareAddress + bytesWithinRegion;
	virtualRegion.virtualIndex = region.virtualIndex + bytesWithinRegion;
	virtualRegion.itemIndex = itemIndex;
	return;
}

template<typename stepType, typename compareType>
unsigned int ItemIndexToVirtualIndex(unsigned int itemIndex)
{
	MemoryRegion virtualRegion;
	ItemIndexToVirtualRegion<stepType,compareType>(itemIndex, virtualRegion);
	return virtualRegion.virtualIndex;
}

template<typename T>
T ReadLocalValue(const unsigned char* data)
{
	return *(const T*)data;
}
//template<> signed char ReadLocalValue(const unsigned char* data) { return *data; }
//template<> unsigned char ReadLocalValue(const unsigned char* data) { return *data; }


template<typename stepType, typename compareType>
compareType GetPrevValueFromVirtualIndex(unsigned int virtualIndex)
{
	return ReadLocalValue<compareType>(s_prevValues + virtualIndex);
	//return *(compareType*)(s_prevValues+virtualIndex);
}
template<typename stepType, typename compareType>
compareType GetCurValueFromVirtualIndex(unsigned int virtualIndex)
{
	return ReadLocalValue<compareType>(s_curValues + virtualIndex);
//	return *(compareType*)(s_curValues+virtualIndex);
}
template<typename stepType, typename compareType>
unsigned short GetNumChangesFromVirtualIndex(unsigned int virtualIndex)
{
	unsigned short num = s_numChanges[virtualIndex];
	//for(unsigned int i = 1; i < sizeof(stepType); i++)
	//	if(num < s_numChanges[virtualIndex+i])
	//		num = s_numChanges[virtualIndex+i];
	return num;
}

template<typename stepType, typename compareType>
compareType GetPrevValueFromItemIndex(unsigned int itemIndex)
{
	int virtualIndex = ItemIndexToVirtualIndex<stepType,compareType>(itemIndex);
	return GetPrevValueFromVirtualIndex<stepType,compareType>(virtualIndex);
}
template<typename stepType, typename compareType>
compareType GetCurValueFromItemIndex(unsigned int itemIndex)
{
	int virtualIndex = ItemIndexToVirtualIndex<stepType,compareType>(itemIndex);
	return GetCurValueFromVirtualIndex<stepType,compareType>(virtualIndex);
}
template<typename stepType, typename compareType>
unsigned short GetNumChangesFromItemIndex(unsigned int itemIndex)
{
	int virtualIndex = ItemIndexToVirtualIndex<stepType,compareType>(itemIndex);
	return GetNumChangesFromVirtualIndex<stepType,compareType>(virtualIndex);
}
template<typename stepType, typename compareType>
unsigned int GetHardwareAddressFromItemIndex(unsigned int itemIndex)
{
	MemoryRegion virtualRegion;
	ItemIndexToVirtualRegion<stepType,compareType>(itemIndex, virtualRegion);
	return virtualRegion.hardwareAddress;
}

// this one might be unreliable, haven't used it much
template<typename stepType, typename compareType>
unsigned int HardwareAddressToItemIndex(HWAddressType hardwareAddress)
{
	if(s_itemIndicesInvalid)
		CalculateItemIndices(sizeof(stepType));

	for(MemoryList::iterator iter = s_activeMemoryRegions.begin(); iter != s_activeMemoryRegions.end(); ++iter)
	{
		MemoryRegion& region = *iter;
		if(hardwareAddress >= region.hardwareAddress && hardwareAddress < region.hardwareAddress + region.size)
		{
			int indexWithinRegion = (hardwareAddress - region.hardwareAddress) / sizeof(stepType);
			return region.itemIndex + indexWithinRegion;
		}
	}

	return -1;
}



// it's ugly but I can't think of a better way to call these functions that isn't also slower, since
// I need the current values of these arguments to determine which primitive types are used within the function
#define CALL_WITH_T_SIZE_TYPES(functionName, sizeTypeID, typeID, requireAligned, ...) \
	(typeID == 'f' \
		? (sizeTypeID == 'l' \
			? (requireAligned \
				? functionName<long long, double>(##__VA_ARGS__) \
				: functionName<char, double>(##__VA_ARGS__)) \
			: (requireAligned \
				? functionName<long, float>(##__VA_ARGS__) \
				: functionName<char, float>(##__VA_ARGS__))) \
	: sizeTypeID == 'b' \
		? (typeID == 's' \
			? functionName<char, signed char>(##__VA_ARGS__) \
			: functionName<char, unsigned char>(##__VA_ARGS__)) \
	: sizeTypeID == 'w' \
		? (typeID == 's' \
			? (requireAligned \
				? functionName<short, signed short>(##__VA_ARGS__) \
				: functionName<char, signed short>(##__VA_ARGS__)) \
			: (requireAligned \
				? functionName<short, unsigned short>(##__VA_ARGS__) \
				: functionName<char, unsigned short>(##__VA_ARGS__))) \
	: sizeTypeID == 'd' \
		? (typeID == 's' \
			? (requireAligned \
				? functionName<long, signed long>(##__VA_ARGS__) \
				: functionName<char, signed long>(##__VA_ARGS__)) \
			: (requireAligned \
				? functionName<long, unsigned long>(##__VA_ARGS__) \
				: functionName<char, unsigned long>(##__VA_ARGS__))) \
	: sizeTypeID == 'l' \
		? (typeID == 's' \
			? (requireAligned \
				? functionName<long long, signed long long>(##__VA_ARGS__) \
				: functionName<char, signed long long>(##__VA_ARGS__)) \
			: (requireAligned \
				? functionName<long long, unsigned long long>(##__VA_ARGS__) \
				: functionName<char, unsigned long long>(##__VA_ARGS__))) \
	: functionName<char, signed char>(##__VA_ARGS__))

// version that takes a forced comparison type
#define CALL_WITH_T_STEP(functionName, sizeTypeID, sign,type, requireAligned, ...) \
	(sizeTypeID == 'b' \
		? functionName<char, sign type>(##__VA_ARGS__) \
	: sizeTypeID == 'w' \
		? (requireAligned \
			? functionName<short, sign type>(##__VA_ARGS__) \
			: functionName<char, sign type>(##__VA_ARGS__)) \
	: sizeTypeID == 'd' \
		? (requireAligned \
			? functionName<long, sign type>(##__VA_ARGS__) \
			: functionName<char, sign type>(##__VA_ARGS__)) \
	: sizeTypeID == 'l' \
		? (requireAligned \
			? functionName<long long, sign type>(##__VA_ARGS__) \
			: functionName<char, sign type>(##__VA_ARGS__)) \
	: functionName<char, sign type>(##__VA_ARGS__))


// basic comparison functions:
template <typename T> inline bool LessCmp (T x, T y, T i)        { return x < y; }
template <typename T> inline bool MoreCmp (T x, T y, T i)        { return x > y; }
template <typename T> inline bool LessEqualCmp (T x, T y, T i)   { return x <= y; }
template <typename T> inline bool MoreEqualCmp (T x, T y, T i)   { return x >= y; }
template <typename T> inline bool EqualCmp (T x, T y, T i)       { return x == y; }
template <typename T> inline bool UnequalCmp (T x, T y, T i)     { return x != y; }
template <typename T> inline bool DiffByCmp (T x, T y, T p)      { return x - y == p || y - x == p; }
template <typename T> inline bool ModIsCmp (T x, T y, T p)       { return p && x % p == y; }
template <> inline bool ModIsCmp (float x, float y, float p)     { return p && fmodf(x, p) == y; }
template <> inline bool ModIsCmp (double x, double y, double p)  { return p && fmod(x, p) == y; }

// compare-to type functions:
template<typename stepType, typename T>
void SearchRelative (bool(*cmpFun)(T,T,T), T ignored, T param)
{
	for(MemoryList::iterator iter = s_activeMemoryRegions.begin(); iter != s_activeMemoryRegions.end(); )
	{
		MemoryRegion& region = *iter;
		int startSkipSize = ((unsigned int)(sizeof(stepType) - region.hardwareAddress)) % sizeof(stepType);
		unsigned int start = region.virtualIndex + startSkipSize;
		unsigned int end = region.virtualIndex + region.size;
		for(unsigned int i = start, hwaddr = region.hardwareAddress; i < end; i += sizeof(stepType), hwaddr += sizeof(stepType))
			if(!cmpFun(GetCurValueFromVirtualIndex<stepType,T>(i), GetPrevValueFromVirtualIndex<stepType,T>(i), param))
				if(2 == DeactivateRegion(region, iter, hwaddr, sizeof(stepType)))
					goto outerContinue;
		++iter;
outerContinue:
		continue;
	}
}
template<typename stepType, typename T>
void SearchSpecific (bool(*cmpFun)(T,T,T), T value, T param)
{
	for(MemoryList::iterator iter = s_activeMemoryRegions.begin(); iter != s_activeMemoryRegions.end(); )
	{
		MemoryRegion& region = *iter;
		int startSkipSize = ((unsigned int)(sizeof(stepType) - region.hardwareAddress)) % sizeof(stepType);
		unsigned int start = region.virtualIndex + startSkipSize;
		unsigned int end = region.virtualIndex + region.size;
		for(unsigned int i = start, hwaddr = region.hardwareAddress; i < end; i += sizeof(stepType), hwaddr += sizeof(stepType))
			if(!cmpFun(GetCurValueFromVirtualIndex<stepType,T>(i), value, param))
				if(2 == DeactivateRegion(region, iter, hwaddr, sizeof(stepType)))
					goto outerContinue;
		++iter;
outerContinue:
		continue;
	}
}
template<typename stepType, typename T>
void SearchAddress (bool(*cmpFun)(T,T,T), T address, T param)
{
	for(MemoryList::iterator iter = s_activeMemoryRegions.begin(); iter != s_activeMemoryRegions.end(); )
	{
		MemoryRegion& region = *iter;
		int startSkipSize = ((unsigned int)(sizeof(stepType) - region.hardwareAddress)) % sizeof(stepType);
		unsigned int start = region.virtualIndex + startSkipSize;
		unsigned int end = region.virtualIndex + region.size;
		for(unsigned int i = start, hwaddr = region.hardwareAddress; i < end; i += sizeof(stepType), hwaddr += sizeof(stepType))
			if(!cmpFun(hwaddr, address, param))
				if(2 == DeactivateRegion(region, iter, hwaddr, sizeof(stepType)))
					goto outerContinue;
		++iter;
outerContinue:
		continue;
	}
}
template<typename stepType, typename T>
void SearchChanges (bool(*cmpFun)(T,T,T), T changes, T param)
{
	for(MemoryList::iterator iter = s_activeMemoryRegions.begin(); iter != s_activeMemoryRegions.end(); )
	{
		MemoryRegion& region = *iter;
		int startSkipSize = ((unsigned int)(sizeof(stepType) - region.hardwareAddress)) % sizeof(stepType);
		unsigned int start = region.virtualIndex + startSkipSize;
		unsigned int end = region.virtualIndex + region.size;
		for(unsigned int i = start, hwaddr = region.hardwareAddress; i < end; i += sizeof(stepType), hwaddr += sizeof(stepType))
			if(!cmpFun(GetNumChangesFromVirtualIndex<stepType,T>(i), changes, param))
				if(2 == DeactivateRegion(region, iter, hwaddr, sizeof(stepType)))
					goto outerContinue;
		++iter;
outerContinue:
		continue;
	}
}


char rs_c='s';
char rs_o='=';
char rs_t='s';
RSVal rs_param=0, rs_val=0;
bool rs_val_valid=false;
char rs_type_size = 'b', rs_last_type_size = rs_type_size;
bool noMisalign = true, rs_last_no_misalign = noMisalign;
//bool littleEndian = false;
int last_rs_possible = -1;
int last_rs_regions = -1;

int sizeTypeIDToSize(char id)
{
	if(id == 'd')
		return 4;
	if(id == 'w')
		return 2;
	if(id == 'l')
		return 8;
	return 1;
}


void prune(char c,char o,char t,RSVal v,RSVal p)
{
	EnterCriticalSection(&s_activeMemoryRegionsCS);

	// repetition-reducing macros
	#define DO_SEARCH(sf) \
	switch (o) \
	{ \
		case '<': DO_SEARCH_2(LessCmp,sf); break; \
		case '>': DO_SEARCH_2(MoreCmp,sf); break; \
		case '=': DO_SEARCH_2(EqualCmp,sf); break; \
		case '!': DO_SEARCH_2(UnequalCmp,sf); break; \
		case 'l': DO_SEARCH_2(LessEqualCmp,sf); break; \
		case 'm': DO_SEARCH_2(MoreEqualCmp,sf); break; \
		case 'd': DO_SEARCH_2(DiffByCmp,sf); break; \
		case '%': DO_SEARCH_2(ModIsCmp,sf); break; \
		default: assert(!"Invalid operator for this search type."); break; \
	}

	// perform the search, eliminating nonmatching values
	switch (c)
	{
		#define DO_SEARCH_2(CmpFun,sf) CALL_WITH_T_SIZE_TYPES(sf, rs_type_size, t, noMisalign, CmpFun,v,p)
		case 'r': DO_SEARCH(SearchRelative); break;
		case 's': DO_SEARCH(SearchSpecific); break;

		#undef DO_SEARCH_2
		#define DO_SEARCH_2(CmpFun,sf) CALL_WITH_T_STEP(sf, rs_type_size, unsigned,int, noMisalign, CmpFun,v,p);
		case 'a': DO_SEARCH(SearchAddress); break;

		#undef DO_SEARCH_2
		#define DO_SEARCH_2(CmpFun,sf) CALL_WITH_T_STEP(sf, rs_type_size, unsigned,short, noMisalign, CmpFun,v,p);
		case 'n': DO_SEARCH(SearchChanges); break;

		default: assert(!"Invalid search comparison type."); break;
	}

	LeaveCriticalSection(&s_activeMemoryRegionsCS);

	s_prevValuesNeedUpdate = true;

	int prevNumItems = last_rs_possible;

	CompactAddrs();

	if(prevNumItems == last_rs_possible)
	{
		SetRamSearchUndoType(RamSearchHWnd, 0); // nothing to undo
	}
}




template<typename stepType, typename T>
bool CompareRelativeAtItem (bool(*cmpFun)(T,T,T), int itemIndex, T ignored, T param)
{
	return cmpFun(GetCurValueFromItemIndex<stepType,T>(itemIndex), GetPrevValueFromItemIndex<stepType,T>(itemIndex), param);
}
template<typename stepType, typename T>
bool CompareSpecificAtItem (bool(*cmpFun)(T,T,T), int itemIndex, T value, T param)
{
	return cmpFun(GetCurValueFromItemIndex<stepType,T>(itemIndex), value, param);
}
template<typename stepType, typename T>
bool CompareAddressAtItem (bool(*cmpFun)(T,T,T), int itemIndex, T address, T param)
{
	return cmpFun(GetHardwareAddressFromItemIndex<stepType,T>(itemIndex), address, param);
}
template<typename stepType, typename T>
bool CompareChangesAtItem (bool(*cmpFun)(T,T,T), int itemIndex, T changes, T param)
{
	return cmpFun(GetNumChangesFromItemIndex<stepType,T>(itemIndex), changes, param);
}

RSVal ReadControlInt(int controlID, char sizeTypeID, char typeID, BOOL& success)
{
	RSVal rv = 0;
	BOOL ok = false;

	char text [64];
	if(GetDlgItemText(RamSearchHWnd,controlID,text,64))
		ok = rv.scan(text, sizeTypeID, typeID);

	success = ok;
	return rv;
}


bool Set_RS_Val()
{
	BOOL success;

	// update rs_val
	switch(rs_c)
	{
		case 'r':
		default:
			rs_val = 0;
			break;
		case 's':
			rs_val = ReadControlInt(IDC_EDIT_COMPAREVALUE, rs_type_size,rs_t, success);
			if(!success)
				return false;
			if((rs_type_size == 'b' && rs_t == 's' && ((int)rs_val < -128 || (int)rs_val > 127)) ||
			   (rs_type_size == 'b' && rs_t != 's' && ((int)rs_val < 0 || (int)rs_val > 255)) ||
			   (rs_type_size == 'w' && rs_t == 's' && ((int)rs_val < -32768 || (int)rs_val > 32767)) ||
			   (rs_type_size == 'w' && rs_t != 's' && ((int)rs_val < 0 || (int)rs_val > 65535)))
			   return false;
			break;
		case 'a':
			rs_val = ReadControlInt(IDC_EDIT_COMPAREADDRESS, 'd','h', success);
			if(!success || (int)rs_val < 0/* || (int)rs_val > 0x06040000*/)
				return false;
			break;
		case 'n': {
			rs_val = ReadControlInt(IDC_EDIT_COMPARECHANGES, 'd','u', success);
			if(!success || (int)rs_val < 0 || (int)rs_val > 0xFFFF)
				return false;
		}	break;
	}

	// also update rs_param
	switch(rs_o)
	{
		default:
			rs_param = 0;
			break;
		case 'd':
			rs_param = ReadControlInt(IDC_EDIT_DIFFBY, (rs_c=='r'||rs_c=='s')?rs_type_size:'d', (rs_c=='r'||rs_c=='s')?rs_t:(rs_c=='a'?'h':'s'), success);
			if(!success)
				return false;
			if((int)rs_param < 0)
				rs_param = -(int)rs_param;
			break;
		case '%':
			rs_param = ReadControlInt(IDC_EDIT_MODBY, (rs_c=='r'||rs_c=='s')?rs_type_size:'d', (rs_c=='r'||rs_c=='s')?rs_t:(rs_c=='a'?'h':'s'), success);
			if(!success || (int)rs_param == 0)
				return false;
			break;
	}

	// validate that rs_param fits in the comparison data type
	{
		int appliedSize = rs_type_size;
		int appliedSign = rs_t;
		if(rs_c == 'n')
			appliedSize = 'w', appliedSign = 'u';
		if(rs_c == 'a')
			appliedSize = 'd', appliedSign = 'u';
		if((appliedSize == 'b' && appliedSize == 's' && ((int)rs_param < -128 || (int)rs_param > 127)) ||
		   (appliedSize == 'b' && appliedSize != 's' && ((int)rs_param < 0 || (int)rs_param > 255)) ||
		   (appliedSize == 'w' && appliedSize == 's' && ((int)rs_param < -32768 || (int)rs_param > 32767)) ||
		   (appliedSize == 'w' && appliedSize != 's' && ((int)rs_param < 0 || (int)rs_param > 65535)))
		   return false;
	}

	return true;
}

bool IsSatisfied(int itemIndex)
{
	if(!rs_val_valid)
		return true;
	int o = rs_o;
	switch (rs_c)
	{
		#undef DO_SEARCH_2
		#define DO_SEARCH_2(CmpFun,sf) return CALL_WITH_T_SIZE_TYPES(sf, rs_type_size,rs_t,noMisalign, CmpFun,itemIndex,rs_val,rs_param);
		case 'r': DO_SEARCH(CompareRelativeAtItem); break;
		case 's': DO_SEARCH(CompareSpecificAtItem); break;

		#undef DO_SEARCH_2
		#define DO_SEARCH_2(CmpFun,sf) return CALL_WITH_T_STEP(sf, rs_type_size, unsigned,int, noMisalign, CmpFun,itemIndex,rs_val,rs_param);
		case 'a': DO_SEARCH(CompareAddressAtItem); break;

		#undef DO_SEARCH_2
		#define DO_SEARCH_2(CmpFun,sf) return CALL_WITH_T_STEP(sf, rs_type_size, unsigned,short, noMisalign, CmpFun,itemIndex,rs_val,rs_param);
		case 'n': DO_SEARCH(CompareChangesAtItem); break;
	}
	return false;
}



RSVal ReadValueAtSoftwareAddress(const unsigned char* address, char sizeTypeID, char typeID)
{
	RSVal value = 0;
	ReadProcessMemory(hGameProcess, (const void*)address, (void*)&value, sizeTypeIDToSize(sizeTypeID), NULL);
	if(typeID == 'f')
		if(sizeTypeID == 'l')
			value.t = RSVal::t_d;
		else
			value.t = RSVal::t_f;
	else
		if(sizeTypeID == 'l')
			value.t = RSVal::t_ll;
		else
			value.t = RSVal::t_i;
	return value;
}
void WriteValueAtSoftwareAddress(unsigned char* address, RSVal value, char sizeTypeID, char typeID)
{
	WriteProcessMemory(hGameProcess, (void*)address, (const void*)&value, sizeTypeIDToSize(sizeTypeID), NULL);
}
RSVal ReadValueAtHardwareAddress(HWAddressType address, char sizeTypeID, char typeID)
{
	return ReadValueAtSoftwareAddress((const unsigned char*)address, sizeTypeID, typeID);
}
bool WriteValueAtHardwareAddress(HWAddressType address, RSVal value, char sizeTypeID, char typeID, bool hookless)
{
	WriteValueAtSoftwareAddress((unsigned char*)address, value, sizeTypeID, typeID);
	//if(!hookless) // a script that calls e.g. memory.writebyte() should trigger write hooks
	//	CallRegisteredLuaMemHook(address, size, value, LUAMEMHOOK_WRITE);
	return true;
}
bool IsHardwareAddressValid(HWAddressType address)
{
	char temp [4];
	return ReadProcessMemory(hGameProcess, (const void*)address, (void*)temp, 1, NULL)
		&& WriteProcessMemory(hGameProcess, (void*)address, (const void*)temp, 1, NULL);
}



int ResultCount=0;
bool AutoSearch=false;
bool AutoSearchAutoRetry=false;
LRESULT CALLBACK PromptWatchNameProc(HWND, UINT, WPARAM, LPARAM);
void UpdatePossibilities(int rs_possible, int regions);


void CompactAddrs()
{
	int size = noMisalign ? sizeTypeIDToSize(rs_type_size) : 1;
	int prevResultCount = ResultCount;

	CalculateItemIndices(size);
	ResultCount = CALL_WITH_T_SIZE_TYPES(CountRegionItemsT, rs_type_size,rs_t,noMisalign);

	UpdatePossibilities(ResultCount, (int)s_activeMemoryRegions.size());

	if(ResultCount != prevResultCount)
		ListView_SetItemCount(GetDlgItem(RamSearchHWnd,IDC_RAMLIST),ResultCount);
}

void soft_reset_address_info ()
{
	s_prevValuesNeedUpdate = false;
	ResetMemoryRegions();
	if(!RamSearchHWnd)
	{
		s_activeMemoryRegions.clear();
		ResultCount = 0;
	}
	else
	{
		// force s_prevValues to be valid
		signal_new_frame();
		s_prevValuesNeedUpdate = true;
		signal_new_frame();
	}
	if(s_numChanges)
		memset(s_numChanges, 0, (sizeof(*s_numChanges)*(MAX_RAM_SIZE)));
	CompactAddrs();
}
void reset_address_info ()
{
	SetRamSearchUndoType(RamSearchHWnd, 0);
	EnterCriticalSection(&s_activeMemoryRegionsCS);
	s_activeMemoryRegionsBackup.clear(); // not necessary, but we'll take the time hit here instead of at the next thing that sets up an undo
	LeaveCriticalSection(&s_activeMemoryRegionsCS);
	if(s_prevValues)
		memcpy(s_prevValues, s_curValues, (sizeof(*s_prevValues)*(MAX_RAM_SIZE)));
	s_prevValuesNeedUpdate = false;
	ResetMemoryRegions();
	if(!RamSearchHWnd)
	{
		EnterCriticalSection(&s_activeMemoryRegionsCS);
		s_activeMemoryRegions.clear();
		LeaveCriticalSection(&s_activeMemoryRegionsCS);
		ResultCount = 0;
	}
	else
	{
		// force s_prevValues to be valid
		signal_new_frame();
		s_prevValuesNeedUpdate = true;
		signal_new_frame();
	}
	memset(s_numChanges, 0, (sizeof(*s_numChanges)*(MAX_RAM_SIZE)));
	CompactAddrs();
}

void signal_new_frame ()
{
	EnterCriticalSection(&s_activeMemoryRegionsCS);
	EnterCriticalSection(&g_processMemCS);
	CALL_WITH_T_SIZE_TYPES(UpdateRegionsT, rs_type_size,rs_t,noMisalign);
	LeaveCriticalSection(&g_processMemCS);
	LeaveCriticalSection(&s_activeMemoryRegionsCS);
}





bool RamSearchClosed = false;
bool RamWatchClosed = false;

void ResetResults()
{
	reset_address_info();
	ResultCount = 0;
	if (RamSearchHWnd)
		ListView_SetItemCount(GetDlgItem(RamSearchHWnd,IDC_RAMLIST),ResultCount);
}
void CloseRamWindows() //Close the Ram Search & Watch windows when rom closes
{
	ResetWatches();
	ResetResults();
	if (RamSearchHWnd)
	{
		SendMessage(RamSearchHWnd,WM_CLOSE,NULL,NULL);
		RamSearchClosed = true;
	}
	if (RamWatchHWnd)
	{
		SendMessage(RamWatchHWnd,WM_CLOSE,NULL,NULL);
		RamWatchClosed = true;
	}
}
void ReopenRamWindows() //Reopen them when a new Rom is loaded
{
	HWND hwnd = GetActiveWindow();

	if (RamSearchClosed)
	{
		RamSearchClosed = false;
		if(!RamSearchHWnd)
		{
			reset_address_info(); // TODO: is this prone to deadlock? should we set ResultCount = 0 instead?
			LRESULT CALLBACK RamSearchProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
			RamSearchHWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_RAMSEARCH), hWnd, (DLGPROC) RamSearchProc);
		}
	}
	if (RamWatchClosed || AutoRWLoad)
	{
		RamWatchClosed = false;
		if(!RamWatchHWnd)
		{
			if (AutoRWLoad) OpenRWRecentFile(0);
			LRESULT CALLBACK RamWatchProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
			RamWatchHWnd = CreateDialog(hInst, MAKEINTRESOURCE(IDD_RAMWATCH), hWnd, (DLGPROC) RamWatchProc);
		}
	}

	if(hwnd == hWnd && hwnd != GetActiveWindow())
		SetActiveWindow(hWnd); // restore focus to the main window if it had it before
}






void RefreshRamListSelectedCountControlStatus(HWND hDlg)
{
	static int prevSelCount=-1;
	int selCount = ListView_GetSelectedCount(GetDlgItem(hDlg,IDC_RAMLIST));
	if(selCount != prevSelCount)
	{
		if(selCount < 2 || prevSelCount < 2)
		{
			EnableWindow(GetDlgItem(hDlg, IDC_C_WATCH), (selCount >= 1 && WatchCount < MAX_WATCH_COUNT) ? TRUE : FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_C_ADDCHEAT), (selCount >= 1) ? /*TRUE*/FALSE : FALSE);
			EnableWindow(GetDlgItem(hDlg, IDC_C_ELIMINATE), (selCount >= 1) ? TRUE : FALSE);
		}
		prevSelCount = selCount;
	}
}




struct AddrRange
{
	unsigned int addr;
	unsigned int size;
	unsigned int End() const { return addr + size; }
	AddrRange(unsigned int a, unsigned int s) : addr(a),size(s){}
};

void signal_new_size ()
{
	HWND lv = GetDlgItem(RamSearchHWnd,IDC_RAMLIST);

	int oldSize = rs_last_no_misalign ? sizeTypeIDToSize(rs_last_type_size) : 1;
	int newSize = noMisalign ? sizeTypeIDToSize(rs_type_size) : 1;
	bool numberOfItemsChanged = (oldSize != newSize);

	unsigned int itemsPerPage = ListView_GetCountPerPage(lv);
	unsigned int oldTopIndex = ListView_GetTopIndex(lv);
	unsigned int oldSelectionIndex = ListView_GetSelectionMark(lv);
	unsigned int oldTopAddr = CALL_WITH_T_SIZE_TYPES(GetHardwareAddressFromItemIndex, rs_last_type_size,rs_t,rs_last_no_misalign, oldTopIndex);
	unsigned int oldSelectionAddr = CALL_WITH_T_SIZE_TYPES(GetHardwareAddressFromItemIndex, rs_last_type_size,rs_t,rs_last_no_misalign, oldSelectionIndex);

	std::vector<AddrRange> selHardwareAddrs;
	if(numberOfItemsChanged)
	{
		// store selection ranges
		// unfortunately this can take a while if the user has a huge range of items selected
//		Clear_Sound_Buffer();
		int selCount = ListView_GetSelectedCount(lv);
		int size = rs_last_no_misalign ? sizeTypeIDToSize(rs_last_type_size) : 1;
		int watchIndex = -1;
		for(int i = 0; i < selCount; ++i)
		{
			watchIndex = ListView_GetNextItem(lv, watchIndex, LVNI_SELECTED);
			int addr = CALL_WITH_T_SIZE_TYPES(GetHardwareAddressFromItemIndex, rs_last_type_size,rs_t,rs_last_no_misalign, watchIndex);
			if(!selHardwareAddrs.empty() && addr == selHardwareAddrs.back().End())
				selHardwareAddrs.back().size += size;
			else if (!(noMisalign && oldSize < newSize && (addr & (newSize-1)) != 0))
				selHardwareAddrs.push_back(AddrRange(addr,size));
		}
	}

	CompactAddrs();

	rs_last_type_size = rs_type_size;
	rs_last_no_misalign = noMisalign;

	if(numberOfItemsChanged)
	{
		// restore selection ranges
		unsigned int newTopIndex = CALL_WITH_T_SIZE_TYPES(HardwareAddressToItemIndex, rs_type_size,rs_t,noMisalign, oldTopAddr);
		unsigned int newBottomIndex = newTopIndex + itemsPerPage - 1;
		SendMessage(lv, WM_SETREDRAW, FALSE, 0);
		ListView_SetItemState(lv, -1, 0, LVIS_SELECTED|LVIS_FOCUSED); // deselect all
		for(unsigned int i = 0; i < selHardwareAddrs.size(); i++)
		{
			// calculate index ranges of this selection
			const AddrRange& range = selHardwareAddrs[i];
			int selRangeTop = CALL_WITH_T_SIZE_TYPES(HardwareAddressToItemIndex, rs_type_size,rs_t,noMisalign, range.addr);
			int selRangeBottom = -1;
			for(int endAddr = range.End()-1; endAddr >= selRangeTop && selRangeBottom == -1; endAddr--)
				selRangeBottom = CALL_WITH_T_SIZE_TYPES(HardwareAddressToItemIndex, rs_type_size,rs_t,noMisalign, endAddr);
			if(selRangeBottom == -1)
				selRangeBottom = selRangeTop;
			if(selRangeTop == -1)
				continue;

			//// select the entire range at once without deselecting the other ranges
			//// looks hacky but it works, and the only documentation I found on how to do this was blatantly false and equally hacky anyway
			//POINT pos;
			//ListView_EnsureVisible(lv, selRangeTop, 0);
			//ListView_GetItemPosition(lv, selRangeTop, &pos);
			//SendMessage(lv, WM_LBUTTONDOWN, MK_LBUTTON|MK_CONTROL, MAKELONG(pos.x,pos.y));
			//ListView_EnsureVisible(lv, selRangeBottom, 0);
			//ListView_GetItemPosition(lv, selRangeBottom, &pos);
			//SendMessage(lv, WM_LBUTTONDOWN, MK_LBUTTON|MK_CONTROL|MK_SHIFT, MAKELONG(pos.x,pos.y));

			// select the entire range
			for (int j = selRangeTop; j <= selRangeBottom; j++)
			{
				ListView_SetItemState(lv, j, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
			}
		}

		// restore previous scroll position
		if(newBottomIndex != -1)
			ListView_EnsureVisible(lv, newBottomIndex, 0);
		if(newTopIndex != -1)
			ListView_EnsureVisible(lv, newTopIndex, 0);

		SendMessage(lv, WM_SETREDRAW, TRUE, 0);

		RefreshRamListSelectedCountControlStatus(RamSearchHWnd);

		EnableWindow(GetDlgItem(RamSearchHWnd,IDC_MISALIGN), rs_type_size != 'b');
	}
	else
	{
		ListView_Update(lv, -1);
	}
	InvalidateRect(lv, NULL, TRUE);
}




LRESULT CustomDraw (LPARAM lParam)
{
	LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;

	switch(lplvcd->nmcd.dwDrawStage) 
	{
		case CDDS_PREPAINT :
			return CDRF_NOTIFYITEMDRAW;

		case CDDS_ITEMPREPAINT:
		{
			int rv = CDRF_DODEFAULT;

			if(lplvcd->nmcd.dwItemSpec % 2)
			{
				// alternate the background color slightly
				lplvcd->clrTextBk = RGB(248,248,255);
				rv = CDRF_NEWFONT;
			}

			if(!IsSatisfied(lplvcd->nmcd.dwItemSpec))
			{
				// tint red any items that would be eliminated if a search were to run now
				lplvcd->clrText = RGB(192,64,64);
				rv = CDRF_NEWFONT;
			}

			return rv;
		}	break;
	}
	return CDRF_DODEFAULT;
}

void Update_RAM_Search() //keeps RAM values up to date in the search and watch windows
{
	if(disableRamSearchUpdate)
		return;

	extern BOOL fastForwardFlags;
	extern bool fastforward;
	extern bool recoveringStale;
	if(fastforward && (RamWatchHWnd || last_rs_possible > 10000 || recoveringStale) && (fastForwardFlags & /*FFMODE_RAMSKIP*/0x08))
	{
		static int count = 0;

		if(recoveringStale)
		{
			if(++count % 128)
				return;
		}
		else
		{
			if(++count % 32)
				return;
		}
	}

	int prevValuesNeededUpdate;
	if (AutoSearch && !ResultCount)
	{
		if(!AutoSearchAutoRetry)
		{
//			Clear_Sound_Buffer();
			int answer = MessageBox(RamSearchHWnd,"Choosing Retry will reset the search once and continue autosearching.\nChoose Ignore will reset the search whenever necessary and continue autosearching.\nChoosing Abort will reset the search once and stop autosearching.","Autosearch - out of results.",MB_ABORTRETRYIGNORE|MB_DEFBUTTON2|MB_ICONINFORMATION);
			if(answer == IDABORT)
			{
				SendDlgItemMessage(RamSearchHWnd, IDC_C_AUTOSEARCH, BM_SETCHECK, BST_UNCHECKED, 0);
				SendMessage(RamSearchHWnd, WM_COMMAND, IDC_C_AUTOSEARCH, 0);
			}
			if(answer == IDIGNORE)
				AutoSearchAutoRetry = true;
		}
		reset_address_info();
		prevValuesNeededUpdate = s_prevValuesNeedUpdate;
	}
	else
	{
		prevValuesNeededUpdate = s_prevValuesNeedUpdate;
		if (RamSearchHWnd)
		{
			// update active RAM values
			signal_new_frame();
		}

		if (AutoSearch && ResultCount)
		{
			//Clear_Sound_Buffer();
			if(!rs_val_valid)
				rs_val_valid = Set_RS_Val();
			if(rs_val_valid)
				prune(rs_c,rs_o,rs_t,rs_val,rs_param);
		}
	}

	if(RamSearchHWnd)
	{
		HWND lv = GetDlgItem(RamSearchHWnd,IDC_RAMLIST);
		if(prevValuesNeededUpdate != s_prevValuesNeedUpdate)
		{
			// previous values got updated, refresh everything visible
			ListView_Update(lv, -1);
		}
		else if(s_numChanges)
		{
			// refresh any visible parts of the listview box that changed
			static int changes[128];
			int top = ListView_GetTopIndex(lv);
			int count = ListView_GetCountPerPage(lv);
			int start = -1;
			for(int i = top; i <= top+count; i++)
			{
				int changeNum = CALL_WITH_T_SIZE_TYPES(GetNumChangesFromItemIndex, rs_type_size,rs_t,noMisalign, i); //s_numChanges[i];
				int changed = changeNum != changes[i-top];
				if(changed)
					changes[i-top] = changeNum;

				if(start == -1)
				{
					if(i != top+count && changed)
					{
						start = i;
						//somethingChanged = true;
					}
				}
				else
				{
					if(i == top+count || !changed)
					{
						ListView_RedrawItems(lv, start, i-1);
						start = -1;
					}
				}
			}
		}
	}

	if(RamWatchHWnd)
	{
		Update_RAM_Watch();
	}
}

static int rs_lastPercent = -1;
inline void UpdateRamSearchProgressBar(int percent)
{
	if(rs_lastPercent != percent)
	{
		rs_lastPercent = percent;
		UpdateRamSearchTitleBar(percent);
	}
}

static void SelectEditControl(int controlID)
{
	HWND hEdit = GetDlgItem(RamSearchHWnd,controlID);
	SetFocus(hEdit);
	SendMessage(hEdit, EM_SETSEL, 0, -1);
}

static BOOL SelectingByKeyboard()
{
	int a = GetKeyState(VK_LEFT);
	int b = GetKeyState(VK_RIGHT);
	int c = GetKeyState(VK_UP);
	int d = GetKeyState(VK_DOWN); // space and tab are intentionally omitted
	return (a | b | c | d) & 0x80;
}


LRESULT CALLBACK RamSearchProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT r;
	RECT r2;
	int dx1, dy1, dx2, dy2;
	static int watchIndex=0;

	switch(uMsg)
	{
		case WM_INITDIALOG: {
			RamSearchHWnd = hDlg;

			GetWindowRect(hWnd, &r);
			dx1 = (r.right - r.left) / 2;
			dy1 = (r.bottom - r.top) / 2;

			GetWindowRect(hDlg, &r2);
			dx2 = (r2.right - r2.left) / 2;
			dy2 = (r2.bottom - r2.top) / 2;

			// push it away from the main window if we can
			const int width = (r.right-r.left); 
			const int width2 = (r2.right-r2.left); 
			if(r.left+width2 + width < GetSystemMetrics(SM_CXSCREEN))
			{
				r.right += width;
				r.left += width;
			}
			else if((int)r.left - (int)width2 > 0)
			{
				r.right -= width2;
				r.left -= width2;
			}

			SetWindowPos(hDlg, NULL, r.left, r.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
			switch(rs_o)
			{
				case '<':
					SendDlgItemMessage(hDlg, IDC_LESSTHAN, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case '>':
					SendDlgItemMessage(hDlg, IDC_MORETHAN, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'l':
					SendDlgItemMessage(hDlg, IDC_NOMORETHAN, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'm':
					SendDlgItemMessage(hDlg, IDC_NOLESSTHAN, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case '=': 
					SendDlgItemMessage(hDlg, IDC_EQUALTO, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case '!':
					SendDlgItemMessage(hDlg, IDC_DIFFERENTFROM, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'd':
					SendDlgItemMessage(hDlg, IDC_DIFFERENTBY, BM_SETCHECK, BST_CHECKED, 0);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_DIFFBY),true);
					break;
				case '%':
					SendDlgItemMessage(hDlg, IDC_MODULO, BM_SETCHECK, BST_CHECKED, 0);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_MODBY),true);
					break;
			}
			switch (rs_c)
			{
				case 'r':
					SendDlgItemMessage(hDlg, IDC_PREVIOUSVALUE, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 's':
					SendDlgItemMessage(hDlg, IDC_SPECIFICVALUE, BM_SETCHECK, BST_CHECKED, 0);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREVALUE),true);
					break;
				case 'a':
					SendDlgItemMessage(hDlg, IDC_SPECIFICADDRESS, BM_SETCHECK, BST_CHECKED, 0);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREADDRESS),true);
					break;
				case 'n':
					SendDlgItemMessage(hDlg, IDC_NUMBEROFCHANGES, BM_SETCHECK, BST_CHECKED, 0);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPARECHANGES),true);
					break;
			}
			switch (rs_t)
			{
				case 's':
					SendDlgItemMessage(hDlg, IDC_SIGNED, BM_SETCHECK, BST_CHECKED, 0);
					EnableWindow(GetDlgItem(hDlg,IDC_1_BYTE),true);
					EnableWindow(GetDlgItem(hDlg,IDC_2_BYTES),true);
					break;
				case 'u':
					SendDlgItemMessage(hDlg, IDC_UNSIGNED, BM_SETCHECK, BST_CHECKED, 0);
					EnableWindow(GetDlgItem(hDlg,IDC_1_BYTE),true);
					EnableWindow(GetDlgItem(hDlg,IDC_2_BYTES),true);
					break;
				case 'h':
					SendDlgItemMessage(hDlg, IDC_HEX, BM_SETCHECK, BST_CHECKED, 0);
					EnableWindow(GetDlgItem(hDlg,IDC_1_BYTE),true);
					EnableWindow(GetDlgItem(hDlg,IDC_2_BYTES),true);
					break;
				case 'f':
					SendDlgItemMessage(hDlg, IDC_FLOAT, BM_SETCHECK, BST_CHECKED, 0);
					EnableWindow(GetDlgItem(hDlg,IDC_1_BYTE),false);
					EnableWindow(GetDlgItem(hDlg,IDC_2_BYTES),false);
					break;
			}
			switch (rs_type_size)
			{
				case 'b':
					SendDlgItemMessage(hDlg, IDC_1_BYTE, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'w':
					SendDlgItemMessage(hDlg, IDC_2_BYTES, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'd':
					SendDlgItemMessage(hDlg, IDC_4_BYTES, BM_SETCHECK, BST_CHECKED, 0);
					break;
				case 'l':
					SendDlgItemMessage(hDlg, IDC_8_BYTES, BM_SETCHECK, BST_CHECKED, 0);
					break;
			}

			s_prevValuesNeedUpdate = true;

			SendDlgItemMessage(hDlg,IDC_C_AUTOSEARCH,BM_SETCHECK,AutoSearch?BST_CHECKED:BST_UNCHECKED,0);
			//const char* names[5] = {"Address","Value","Previous","Changes","Notes"};
			//int widths[5] = {62,64,64,55,55};
			const char* names[] = {"Address","Value","Previous","Changes"};
			int widths[4] = {68,76,76,68};
			if (!ResultCount)
				reset_address_info();
			else
			{
				signal_new_frame();
				CompactAddrs();
			}
			void init_list_box(HWND Box, const char* Strs[], int numColumns, int *columnWidths);
			init_list_box(GetDlgItem(hDlg,IDC_RAMLIST),names,4,widths);
			//ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),ResultCount);
			if (!noMisalign) SendDlgItemMessage(hDlg, IDC_MISALIGN, BM_SETCHECK, BST_CHECKED, 0);
			//if (littleEndian) SendDlgItemMessage(hDlg, IDC_ENDIAN, BM_SETCHECK, BST_CHECKED, 0);
			last_rs_possible = -1;
			RefreshRamListSelectedCountControlStatus(hDlg);

			// force misalign checkbox to refresh
			signal_new_size();

			// force undo button to refresh
			int undoType = s_undoType;
			SetRamSearchUndoType(hDlg, -2);
			SetRamSearchUndoType(hDlg, undoType);

			// force possibility count to refresh
			last_rs_possible--;
			UpdatePossibilities(ResultCount, (int)s_activeMemoryRegions.size());
			
			rs_val_valid = Set_RS_Val();

			ListView_SetCallbackMask(GetDlgItem(hDlg,IDC_RAMLIST), LVIS_FOCUSED|LVIS_SELECTED);

			return true;
		}	break;

		case WM_NOTIFY:
		{
			LPNMHDR lP = (LPNMHDR) lParam;
			switch (lP->code)
			{
				case LVN_ITEMCHANGED: // selection changed event
				{
					NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)lP;
					if(pNMListView->uNewState & LVIS_FOCUSED ||
						(pNMListView->uNewState ^ pNMListView->uOldState) & LVIS_SELECTED)
					{
						// disable buttons that we don't have the right number of selected items for
						RefreshRamListSelectedCountControlStatus(hDlg);
					}
				}	break;

				case LVN_GETDISPINFO:
				{
					LV_DISPINFO *Item = (LV_DISPINFO *)lParam;
					Item->item.mask = LVIF_TEXT;
					Item->item.state = 0;
					Item->item.iImage = 0;
					const unsigned int iNum = Item->item.iItem;
					static char num[64];
					switch (Item->item.iSubItem)
					{
						case 0:
						{
							int addr = CALL_WITH_T_SIZE_TYPES(GetHardwareAddressFromItemIndex, rs_type_size,rs_t,noMisalign, iNum);
							sprintf(num,"%08X",addr);
							Item->item.pszText = num;
						}	return true;
						case 1:
						{
							RSVal rsval = CALL_WITH_T_SIZE_TYPES((RSVal)GetCurValueFromItemIndex, rs_type_size,rs_t,noMisalign, iNum);
							rsval.print(num, rs_type_size, rs_t);
							Item->item.pszText = num;
						}	return true;
						case 2:
						{
							RSVal rsval = CALL_WITH_T_SIZE_TYPES((RSVal)GetPrevValueFromItemIndex, rs_type_size,rs_t,noMisalign, iNum);
							rsval.print(num, rs_type_size, rs_t);
							Item->item.pszText = num;
						}	return true;
						case 3:
						{
							int i = CALL_WITH_T_SIZE_TYPES(GetNumChangesFromItemIndex, rs_type_size,rs_t,noMisalign, iNum);
							sprintf(num,"%d",i);
							Item->item.pszText = num;
						}	return true;
						//case 4:
						//	Item->item.pszText = rsaddrs[rsresults[iNum].Index].comment ? rsaddrs[rsresults[iNum].Index].comment : "";
						//	return true;
						default:
							return false;
					}
				}

				case NM_CUSTOMDRAW:
				{
					SetWindowLong(hDlg, DWL_MSGRESULT, CustomDraw(lParam));
					return TRUE;
				}	break;

				//case LVN_ODCACHEHINT: //Copied this bit from the MSDN virtual listbox code sample. Eventually it should probably do something.
				//{
				//	LPNMLVCACHEHINT   lpCacheHint = (LPNMLVCACHEHINT)lParam;
				//	return 0;
				//}
				//case LVN_ODFINDITEM: //Copied this bit from the MSDN virtual listbox code sample. Eventually it should probably do something.
				//{	
				//	LPNMLVFINDITEM lpFindItem = (LPNMLVFINDITEM)lParam;
				//	return 0;
				//}
			}
		}	break;

		case WM_COMMAND:
		{
			int rv = false;
			switch(LOWORD(wParam))
			{
				case IDC_SIGNED:
					rs_t='s';
					signal_new_size();
					EnableWindow(GetDlgItem(hDlg,IDC_1_BYTE),true);
					EnableWindow(GetDlgItem(hDlg,IDC_2_BYTES),true);
					{rv = true; break;}
				case IDC_UNSIGNED:
					rs_t='u';
					signal_new_size();
					EnableWindow(GetDlgItem(hDlg,IDC_1_BYTE),true);
					EnableWindow(GetDlgItem(hDlg,IDC_2_BYTES),true);
					{rv = true; break;}
				case IDC_HEX:
					rs_t='h';
					signal_new_size();
					EnableWindow(GetDlgItem(hDlg,IDC_1_BYTE),true);
					EnableWindow(GetDlgItem(hDlg,IDC_2_BYTES),true);
					{rv = true; break;}
				case IDC_FLOAT:
					rs_t='f';
					if(rs_type_size == 'b' || rs_type_size == 'w')
					{
						SendDlgItemMessage(hDlg, IDC_4_BYTES, BM_SETCHECK, BST_CHECKED, 0);
						SendDlgItemMessage(hDlg, IDC_8_BYTES, BM_SETCHECK, BST_UNCHECKED, 0);
						SendDlgItemMessage(hDlg, IDC_2_BYTES, BM_SETCHECK, BST_UNCHECKED, 0);
						SendDlgItemMessage(hDlg, IDC_1_BYTE, BM_SETCHECK, BST_UNCHECKED, 0);
						rs_type_size = 'd';
					}
					EnableWindow(GetDlgItem(hDlg,IDC_1_BYTE),false);
					EnableWindow(GetDlgItem(hDlg,IDC_2_BYTES),false);
					signal_new_size();
					{rv = true; break;}
				case IDC_1_BYTE:
					rs_type_size = 'b';
					signal_new_size();
					{rv = true; break;}
				case IDC_2_BYTES:
					rs_type_size = 'w';
					signal_new_size();
					{rv = true; break;}
				case IDC_4_BYTES:
					rs_type_size = 'd';
					signal_new_size();
					{rv = true; break;}
				case IDC_8_BYTES:
					rs_type_size = 'l';
					signal_new_size();
					{rv = true; break;}
				case IDC_MISALIGN:
					noMisalign = !noMisalign;
					//CompactAddrs();
					signal_new_size();
					{rv = true; break;}
//				case IDC_ENDIAN:
////					littleEndian = !littleEndian;
////					signal_new_size();
//					{rv = true; break;}				
				case IDC_LESSTHAN:
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_DIFFBY),false);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_MODBY),false);
					rs_o = '<';
					{rv = true; break;}
				case IDC_MORETHAN:
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_DIFFBY),false);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_MODBY),false);
					rs_o = '>';
					{rv = true; break;}
				case IDC_NOMORETHAN:
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_DIFFBY),false);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_MODBY),false);
					rs_o = 'l';
					{rv = true; break;}
				case IDC_NOLESSTHAN:
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_DIFFBY),false);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_MODBY),false);
					rs_o = 'm';
					{rv = true; break;}
				case IDC_EQUALTO:
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_DIFFBY),false);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_MODBY),false);
					rs_o = '=';
					{rv = true; break;}
				case IDC_DIFFERENTFROM:
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_DIFFBY),false);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_MODBY),false);
					rs_o = '!';
					{rv = true; break;}
				case IDC_DIFFERENTBY:
				{
					rs_o = 'd';
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_DIFFBY),true);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_MODBY),false);
					if(!SelectingByKeyboard())
						SelectEditControl(IDC_EDIT_DIFFBY);
				}	{rv = true; break;}
				case IDC_MODULO:
				{
					rs_o = '%';
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_DIFFBY),false);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_MODBY),true);
					if(!SelectingByKeyboard())
						SelectEditControl(IDC_EDIT_MODBY);
				}	{rv = true; break;}
				case IDC_PREVIOUSVALUE:
					rs_c='r';
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREVALUE),false);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREADDRESS),false);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPARECHANGES),false);
					{rv = true; break;}
				case IDC_SPECIFICVALUE:
				{
					rs_c = 's';
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREVALUE),true);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREADDRESS),false);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPARECHANGES),false);
					if(!SelectingByKeyboard())
						SelectEditControl(IDC_EDIT_COMPAREVALUE);
					{rv = true; break;}
				}
				case IDC_SPECIFICADDRESS:
				{
					rs_c = 'a';
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREADDRESS),true);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREVALUE),false);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPARECHANGES),false);
					if(!SelectingByKeyboard())
						SelectEditControl(IDC_EDIT_COMPAREADDRESS);
				}	{rv = true; break;}
				case IDC_NUMBEROFCHANGES:
				{
					rs_c = 'n';
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPARECHANGES),true);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREVALUE),false);
					EnableWindow(GetDlgItem(hDlg,IDC_EDIT_COMPAREADDRESS),false);
					if(!SelectingByKeyboard())
						SelectEditControl(IDC_EDIT_COMPARECHANGES);
				}	{rv = true; break;}
				case IDC_C_ADDCHEAT:
				{
					//HWND ramListControl = GetDlgItem(hDlg,IDC_RAMLIST);
					//int cheatItemIndex = ListView_GetNextItem(ramListControl, -1, LVNI_SELECTED);
					//while (cheatItemIndex >= 0)
					//{
					//	u32 address = CALL_WITH_T_SIZE_TYPES(GetHardwareAddressFromItemIndex, rs_type_size,rs_t,noMisalign, cheatItemIndex);
					//	u8 size = (rs_type_size=='b') ? 1 : (rs_type_size=='w' ? 2 : 4);
					//	u32 value = CALL_WITH_T_SIZE_TYPES(GetCurValueFromItemIndex, rs_type_size,rs_t,noMisalign, cheatItemIndex);
					//	CheatsAddDialog(hDlg, address, value, size);
					//	cheatItemIndex = ListView_GetNextItem(ramListControl, cheatItemIndex, LVNI_SELECTED);
					//}
					//{rv = true; break;}
				}
				case IDC_C_RESET:
				{
					RamSearchSaveUndoStateIfNotTooBig(RamSearchHWnd);
					int prevNumItems = last_rs_possible;

					soft_reset_address_info();

					if(prevNumItems == last_rs_possible)
						SetRamSearchUndoType(RamSearchHWnd, 0); // nothing to undo

					ListView_SetItemState(GetDlgItem(hDlg,IDC_RAMLIST), -1, 0, LVIS_SELECTED); // deselect all
					//ListView_SetItemCount(GetDlgItem(hDlg,IDC_RAMLIST),ResultCount);
					ListView_SetSelectionMark(GetDlgItem(hDlg,IDC_RAMLIST), 0);
					RefreshRamListSelectedCountControlStatus(hDlg);
					{rv = true; break;}
				}
				case IDC_C_RESET_CHANGES:
					memset(s_numChanges, 0, (sizeof(*s_numChanges)*(MAX_RAM_SIZE)));
					ListView_Update(GetDlgItem(hDlg,IDC_RAMLIST), -1);
					//SetRamSearchUndoType(hDlg, 0);
					{rv = true; break;}
				case IDC_C_UNDO:
					if(s_undoType>0)
					{
//						Clear_Sound_Buffer();
						EnterCriticalSection(&s_activeMemoryRegionsCS);
						if(s_activeMemoryRegions.size() < tooManyRegionsForUndo)
						{
							MemoryList tempMemoryList = s_activeMemoryRegions;
							s_activeMemoryRegions = s_activeMemoryRegionsBackup;
							s_activeMemoryRegionsBackup = tempMemoryList;
							LeaveCriticalSection(&s_activeMemoryRegionsCS);
							SetRamSearchUndoType(hDlg, 3 - s_undoType);
						}
						else
						{
							s_activeMemoryRegions = s_activeMemoryRegionsBackup;
							LeaveCriticalSection(&s_activeMemoryRegionsCS);
							SetRamSearchUndoType(hDlg, -1);
						}
						CompactAddrs();
						ListView_SetItemState(GetDlgItem(hDlg,IDC_RAMLIST), -1, 0, LVIS_SELECTED); // deselect all
						ListView_SetSelectionMark(GetDlgItem(hDlg,IDC_RAMLIST), 0);
						RefreshRamListSelectedCountControlStatus(hDlg);
					}
					{rv = true; break;}
				case IDC_C_AUTOSEARCH:
					AutoSearch = SendDlgItemMessage(hDlg, IDC_C_AUTOSEARCH, BM_GETCHECK, 0, 0) != 0;
					AutoSearchAutoRetry = false;
					if (!AutoSearch) {rv = true; break;}
				case IDC_C_SEARCH:
				{
//					Clear_Sound_Buffer();

					if(!rs_val_valid && !(rs_val_valid = Set_RS_Val()))
						goto invalid_field;

					if(ResultCount)
					{
						RamSearchSaveUndoStateIfNotTooBig(hDlg);

						prune(rs_c,rs_o,rs_t,rs_val,rs_param);

						RefreshRamListSelectedCountControlStatus(hDlg);
					}

					if(!ResultCount)
					{

						MessageBox(RamSearchHWnd,"Resetting search.","Out of results.",MB_OK|MB_ICONINFORMATION);
						soft_reset_address_info();
					}

					{rv = true; break;}

invalid_field:
					MessageBox(RamSearchHWnd,"Invalid or out-of-bound entered value.","Error",MB_OK|MB_ICONSTOP);
					if(AutoSearch) // stop autosearch if it just started
					{
						SendDlgItemMessage(hDlg, IDC_C_AUTOSEARCH, BM_SETCHECK, BST_UNCHECKED, 0);
						SendMessage(hDlg, WM_COMMAND, IDC_C_AUTOSEARCH, 0);
					}
					{rv = true; break;}
				}
				case IDC_C_WATCH:
				{
					HWND ramListControl = GetDlgItem(hDlg,IDC_RAMLIST);
					int selCount = ListView_GetSelectedCount(ramListControl);

					bool inserted = false;
					int watchItemIndex = ListView_GetNextItem(ramListControl, -1, LVNI_SELECTED);
					while (watchItemIndex >= 0)
					{
						AddressWatcher tempWatch;
						tempWatch.Address = CALL_WITH_T_SIZE_TYPES(GetHardwareAddressFromItemIndex, rs_type_size,rs_t,noMisalign, watchItemIndex);
						tempWatch.Size = rs_type_size;
						tempWatch.Type = rs_t;
						tempWatch.WrongEndian = 0; //Replace when I get little endian working
						tempWatch.comment = NULL;

						if (selCount == 1)
							inserted |= InsertWatch(tempWatch, hDlg);
						else
							inserted |= InsertWatch(tempWatch, "");

						watchItemIndex = ListView_GetNextItem(ramListControl, watchItemIndex, LVNI_SELECTED);
					}
					// bring up the ram watch window if it's not already showing so the user knows where the watch went
					if(inserted && !RamWatchHWnd)
						SendMessage(hWnd, WM_COMMAND, ID_RAM_WATCH, 0);
					SetForegroundWindow(RamSearchHWnd);
					{rv = true; break;}
				}

				// eliminate all selected items
				case IDC_C_ELIMINATE:
				{
					RamSearchSaveUndoStateIfNotTooBig(hDlg);

					HWND ramListControl = GetDlgItem(hDlg,IDC_RAMLIST);
					int size = noMisalign ? sizeTypeIDToSize(rs_type_size) : 1;
					int selCount = ListView_GetSelectedCount(ramListControl);
					int watchIndex = -1;

					// time-saving trick #1:
					// condense the selected items into an array of address ranges
					std::vector<AddrRange> selHardwareAddrs;
					for(int i = 0, j = 1024; i < selCount; ++i, --j)
					{
						watchIndex = ListView_GetNextItem(ramListControl, watchIndex, LVNI_SELECTED);
						int addr = CALL_WITH_T_SIZE_TYPES(GetHardwareAddressFromItemIndex, rs_type_size,rs_t,noMisalign, watchIndex);
						if(!selHardwareAddrs.empty() && addr == selHardwareAddrs.back().End())
							selHardwareAddrs.back().size += size;
						else
							selHardwareAddrs.push_back(AddrRange(addr,size));

						if(!j) UpdateRamSearchProgressBar(i * 50 / selCount), j = 1024;
					}

					// now deactivate the ranges

					// time-saving trick #2:
					// take advantage of the fact that the listbox items must be in the same order as the regions
					MemoryList::iterator iter = s_activeMemoryRegions.begin();
					int numHardwareAddrRanges = selHardwareAddrs.size();
					for(int i = 0, j = 16; i < numHardwareAddrRanges; ++i, --j)
					{
						int addr = selHardwareAddrs[i].addr;
						int size = selHardwareAddrs[i].size;
						bool affected = false;
						while(iter != s_activeMemoryRegions.end())
						{
							MemoryRegion& region = *iter;
							int affNow = DeactivateRegion(region, iter, addr, size);
							if(affNow)
								affected = true;
							else if(affected)
								break;
							if(affNow != 2)
								++iter;
						}

						if(!j) UpdateRamSearchProgressBar(50 + (i * 50 / selCount)), j = 16;
					}
					UpdateRamSearchTitleBar();

					// careful -- if the above two time-saving tricks aren't working,
					// the runtime can absolutely explode (seconds -> hours) when there are lots of regions

					ListView_SetItemState(ramListControl, -1, 0, LVIS_SELECTED); // deselect all
					signal_new_size();
					{rv = true; break;}
				}
				//case IDOK:
				case IDCANCEL:
					RamSearchHWnd = NULL;
					EndDialog(hDlg, true);
					{rv = true; break;}
			}

			// check refresh for comparison preview color update
			// also, update rs_val if needed
			bool needRefresh = false;
			switch(LOWORD(wParam))
			{
				case IDC_LESSTHAN:
				case IDC_MORETHAN:
				case IDC_NOMORETHAN:
				case IDC_NOLESSTHAN:
				case IDC_EQUALTO:
				case IDC_DIFFERENTFROM:
				case IDC_DIFFERENTBY:
				case IDC_MODULO:
				case IDC_PREVIOUSVALUE:
				case IDC_SPECIFICVALUE:
				case IDC_SPECIFICADDRESS:
				case IDC_NUMBEROFCHANGES:
				case IDC_SIGNED:
				case IDC_UNSIGNED:
				case IDC_HEX:
				case IDC_FLOAT:
					rs_val_valid = Set_RS_Val();
					needRefresh = true;
					break;
				case IDC_EDIT_COMPAREVALUE:
				case IDC_EDIT_COMPAREADDRESS:
				case IDC_EDIT_COMPARECHANGES:
				case IDC_EDIT_DIFFBY:
				case IDC_EDIT_MODBY:
					if(HIWORD(wParam) == EN_CHANGE)
					{
						rs_val_valid = Set_RS_Val();
						needRefresh = true;
					}
					break;
			}
			if(needRefresh)
				ListView_Update(GetDlgItem(hDlg,IDC_RAMLIST), -1);


			return rv;
		}	break;

		case WM_CLOSE:
			RamSearchHWnd = NULL;
			EndDialog(hDlg, true);
			return true;
	}

	return false;
}

void UpdateRamSearchTitleBar(int percent)
{
#define HEADER_STR " RAM Search - "
#define PROGRESS_STR " %d%% ... "
#define STATUS_STR "%d Possibilit%s (%d Region%s)"

	int poss = last_rs_possible;
	int regions = last_rs_regions;
	if(poss <= 0)
		strcpy(Str_Tmp_RS," RAM Search");
	else if(percent <= 0)
		sprintf(Str_Tmp_RS, HEADER_STR STATUS_STR, poss, poss==1?"y":"ies", regions, regions==1?"":"s");
	else
		sprintf(Str_Tmp_RS, PROGRESS_STR STATUS_STR, percent, poss, poss==1?"y":"ies", regions, regions==1?"":"s");
	SetWindowText(RamSearchHWnd, Str_Tmp_RS);
}

void UpdatePossibilities(int rs_possible, int regions)
{
	if(rs_possible != last_rs_possible)
	{
		last_rs_possible = rs_possible;
		last_rs_regions = regions;
		UpdateRamSearchTitleBar();
	}
}

void SetRamSearchUndoType(HWND hDlg, int type)
{
	if(s_undoType != type)
	{
		if((s_undoType!=2 && s_undoType!=-1)!=(type!=2 && type!=-1))
			SendDlgItemMessage(hDlg,IDC_C_UNDO,WM_SETTEXT,0,(LPARAM)((type == 2 || type == -1) ? "Redo" : "Undo"));
		if((s_undoType>0)!=(type>0))
			EnableWindow(GetDlgItem(hDlg,IDC_C_UNDO),type>0);
		s_undoType = type;
	}
}

void RamSearchSaveUndoStateIfNotTooBig(HWND hDlg)
{
	EnterCriticalSection(&s_activeMemoryRegionsCS);
	if(s_activeMemoryRegions.size() < tooManyRegionsForUndo)
	{
		s_activeMemoryRegionsBackup = s_activeMemoryRegions;
		LeaveCriticalSection(&s_activeMemoryRegionsCS);
		SetRamSearchUndoType(hDlg, 1);
	}
	else
	{
		LeaveCriticalSection(&s_activeMemoryRegionsCS);
		SetRamSearchUndoType(hDlg, 0);
	}
}

void InitRamSearch()
{
	InitializeCriticalSection(&s_activeMemoryRegionsCS);
}

#endif // compiler version


void init_list_box(HWND Box, const char* Strs[], int numColumns, int *columnWidths) //initializes the ram search and/or ram watch listbox
{
	LVCOLUMN Col;
	Col.mask = LVCF_FMT | LVCF_ORDER | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
	Col.fmt = LVCFMT_CENTER;
	for (int i = 0; i < numColumns; i++)
	{
		Col.iOrder = i;
		Col.iSubItem = i;
		Col.pszText = (LPSTR)(Strs[i]);
		Col.cx = columnWidths[i];
		ListView_InsertColumn(Box,i,&Col);
	}

	ListView_SetExtendedListViewStyle(Box, LVS_EX_FULLROWSELECT);
}

void DeallocateRamSearch()
{
	if(RamSearchHWnd)
	{
		ListView_SetItemCount(GetDlgItem(RamSearchHWnd,IDC_RAMLIST),0);
		RefreshRamListSelectedCountControlStatus(RamSearchHWnd);
		last_rs_possible--;
		UpdatePossibilities(0,0);
	}
	s_itemIndicesInvalid = true;
	s_prevValuesNeedUpdate = true;
	s_maxItemIndex = 0;
	MAX_RAM_SIZE = 0;
	s_undoType = 0;
	free(s_prevValues); s_prevValues = 0;
	free(s_curValues); s_curValues = 0;
	free(s_numChanges); s_numChanges = 0;
	free(s_itemIndexToRegionPointer); s_itemIndexToRegionPointer = 0;
	EnterCriticalSection(&s_activeMemoryRegionsCS);
	MemoryList temp1; s_activeMemoryRegions.swap(temp1);
	MemoryList temp2; s_activeMemoryRegionsBackup.swap(temp2);
	LeaveCriticalSection(&s_activeMemoryRegionsCS);
}

#else
#pragma message(__FILE__": (skipped compilation)")
#endif

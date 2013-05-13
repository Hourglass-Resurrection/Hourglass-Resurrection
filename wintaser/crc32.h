/*  Copyright (C) 2011 nitsuja and contributors
    Hourglass is licensed under GPL v2. Full notice is in COPYING.txt. */

#ifndef CRC32_H_INCL
#define CRC32_H_INCL

// based on:
/* 7zCrc.h -- CRC32 calculation   2008-03-13   Igor Pavlov   Public domain */
/* 7zCrc.c -- CRC32 calculation   2008-08-05   Igor Pavlov   Public domain */

#define CRC_INIT_VAL 0xFFFFFFFF
#define CRC_GET_DIGEST(crc) ((crc) ^ 0xFFFFFFFF)
#define CRC_UPDATE_BYTE(crc, b) (crcTable[((crc) ^ (b)) & 0xFF] ^ ((crc) >> 8))
#define kCrcPoly 0xEDB88320

inline __declspec(noinline)
void CrcGenerateTable(DWORD* table)
{
	for(DWORD i = 0; i < 256; i++)
	{
		DWORD r = i;
		int j;
		for (j = 0; j < 8; j++)
			r = (r >> 1) ^ (kCrcPoly & ~((r & 1) - 1));
		table[i] = r;
	}
}

inline __declspec(noinline)
DWORD CrcUpdate(DWORD v, const void* data, size_t size)
{
	static DWORD crcTable [256];
	static bool needInit = true;
	if(needInit) { CrcGenerateTable(crcTable); needInit = false; }

	for(const unsigned char* p = (const unsigned char*)data; size > 0; size--, p++)
		v = CRC_UPDATE_BYTE(v, *p);
	return v;
}

inline
DWORD CrcCalc(const void* data, size_t size)
{
	return CrcUpdate(CRC_INIT_VAL, data, size) ^ 0xFFFFFFFF;
}


#endif

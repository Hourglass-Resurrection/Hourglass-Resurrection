#include <windows.h>

#include "CRCMath.h"

#include <string>
#include <stdio.h>
#include <map>

#include "crc32.h"

DWORD CalcFileCrc(const char* path)
{
	FILE* file = fopen(path, "rb");
	if(!file)
		return 0;
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	fseek(file, 0, SEEK_SET);
	char* data = (char*)malloc(size);
	fread(data, 1, size, file);
	fclose(file);
	DWORD rv = CrcCalc(data, size);
	free(data);
	return rv;
}

static std::map<std::string, DWORD> crcCache;
DWORD CalcFileCrcCached(const char* path)
{
	std::string key = path;
	std::map<std::string, DWORD>::iterator found = crcCache.find(key);
	if(found == crcCache.end())
	{
		DWORD rv = CalcFileCrc(path);
		crcCache.insert(std::make_pair(key, rv));
		return rv;
	}
	else
	{
		return found->second;
	}
}
void ClearCrcCache()
{
	crcCache.clear();
}

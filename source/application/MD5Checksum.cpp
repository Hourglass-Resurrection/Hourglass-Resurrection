#include "MD5Checksum.h"

#include <Windows.h>

#include <map>
#include <stdio.h>
#include <string>

#include "md5.h"

MD5_CTX ctx;

bool CalcFileMD5(LPCWSTR path, unsigned int* result)
{
    MD5_Init(&ctx);

    FILE* file = _wfopen(path, L"rb");
    if (!file)
        return false;
    fseek(file, 0, SEEK_END);
    unsigned int size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* data = new char[size];
    fread(data, 1, size, file);
    fclose(file);

    MD5_Update(&ctx, data, size);

    delete[] data;

    MD5_Final(result, &ctx);
    return true;
}

// TODO: Figure out a better key system.
static std::map<std::wstring, unsigned int*> MD5Cache;
bool CalcFileMD5Cached(LPCWSTR path, unsigned int* result)
{
    std::wstring key = path;
    std::map<std::wstring, unsigned int*>::iterator found = MD5Cache.find(key);
    if (found == MD5Cache.end())
    {
        unsigned int* rv = new unsigned int[4];
        if (CalcFileMD5(path, rv))
        {
            MD5Cache.insert(std::make_pair(key, rv));
            memcpy(result, rv, 4 * 4);
            return true;
        }
        else
            return false;
    }
    else
    {
        memcpy(result, found->second, 4 * 4);
        return true;
    }
}

void ClearMD5Cache()
{
    // Since the unsigned int* is an array, we better delete it's contents too, or we'll have a leak.
    std::map<std::wstring, unsigned int*>::iterator it;
    for (it = MD5Cache.begin(); it != MD5Cache.end(); it++)
    {
        delete[] it->second;
    }
    MD5Cache.clear();
}

#pragma once

#include <Windows.h>

bool CalcFileMD5(LPCWSTR path, unsigned int* result);
bool CalcFileMD5Cached(LPCWSTR path, unsigned int* result);
void ClearMD5Cache();

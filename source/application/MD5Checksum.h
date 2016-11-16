#pragma once

bool CalcFileMD5(LPCWSTR path, unsigned int* result);
bool CalcFileMD5Cached(LPCWSTR path, unsigned int* result);
void ClearMD5Cache();

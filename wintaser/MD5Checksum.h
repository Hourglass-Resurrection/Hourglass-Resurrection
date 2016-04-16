#pragma once

bool CalcFileMD5(const char* path, unsigned int* result);
bool CalcFileMD5Cached(const char* path, unsigned int* result);
void ClearMD5Cache();

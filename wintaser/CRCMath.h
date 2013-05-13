#pragma once

DWORD CalcFileCrc(const char* path);
DWORD CalcFileCrcCached(const char* path);
void ClearCrcCache();
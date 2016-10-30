#pragma once

// Must be called once and ONCE only, recommended to make the call from the initialization code of the whole program!
void InitDebugCriticalSection();

int debugprintf(LPCWSTR fmt, ...);
void PrintLastError(LPCWSTR lpszFunction, DWORD dw);

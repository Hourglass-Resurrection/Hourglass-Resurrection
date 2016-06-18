#pragma once

// Must be called once and ONCE only, recommended to make the call from the initialization code of the whole program!
void InitDebugCriticalSection();

int debugprintf(const char * fmt, ...);
void PrintLastError(LPTSTR lpszFunction, DWORD dw);

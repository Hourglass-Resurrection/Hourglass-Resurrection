#include <windows.h>

#include "logging.h"

#include <stdio.h>
//#include "../shared/logcat.h"
#include "shared/ipc.h"
#include "Config.h"
//#define ANONYMIZE_PRINT_NUMS // for simplifying diffs (debugging)

FILE* debuglogfile = NULL;

CRITICAL_SECTION g_debugPrintCS;

//extern TasFlags localTASflags;

void InitDebugCriticalSection()
{
	InitializeCriticalSection(&g_debugPrintCS);
}

int debugprintf(LPCWSTR fmt, ...)
{
	WCHAR str[4096];
	va_list args;
	va_start (args, fmt);
	int rv = vswprintf (str, fmt, args);
	va_end (args);
#ifdef ANONYMIZE_PRINT_NUMS
	{
		char* pstr = str;
		while(char c = *pstr)
		{
			if(c == '0')
			{
				while(char c2 = *pstr)
				{
					if(c2 >= '0' && c2 <= '9'
					|| c2 >= 'a' && c2 <= 'f'
					|| c2 >= 'A' && c2 <= 'F'
					|| c2 == 'x' || c2 == 'X')
					{
						*pstr = 'X';
					}
					else
					{
						break;
					}
					pstr++;
				}
			}
			pstr++;
		}
	}
#endif
    if (IsDebuggerPresent())
    {
        OutputDebugStringW(str);
    }
	EnterCriticalSection(&g_debugPrintCS);
    if (!debuglogfile)
    {
        debuglogfile = fopen("hourglasslog.txt", "w");
        if (debuglogfile)
        {
            fwide(debuglogfile, 1);
        }
    }
	if(debuglogfile)
	{
		fputws(str, debuglogfile);
		fflush(debuglogfile);
	}
	LeaveCriticalSection(&g_debugPrintCS);
	return rv;
}

void PrintLastError(LPCWSTR lpszFunction, DWORD dw)
{
	if(!dw)
		return;

	LPVOID lpMsgBuf;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&lpMsgBuf, 0, NULL );

	debugprintf(L"%s failed, error %d: %s", lpszFunction, dw, lpMsgBuf);
    LocalFree(lpMsgBuf);
}

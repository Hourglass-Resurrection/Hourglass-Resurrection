//==========================================
// minicrt - Chris Benshoof 2009
// sprintf()
//==========================================
#include "libctiny.h"
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

// Force the linker to include USER32.LIB
#pragma comment(linker, "/defaultlib:user32.lib")

extern "C" int __cdecl sprintf (char *string, const char *format, ...)
{
    int retValue;
    va_list argptr;

    va_start( argptr, format );
    retValue = wvsprintf( string, format, argptr );
    va_end( argptr );

    return retValue;
}


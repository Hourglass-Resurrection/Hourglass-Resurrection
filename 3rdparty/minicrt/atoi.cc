//==========================================
// minicrt - Chris Benshoof 2009
// atoi(), modified from 
// http://research.microsoft.com/en-us/um/redmond/projects/invisible/src/crt/atoi.c.htm
//==========================================
#include "libctiny.h"

extern "C" int __cdecl atoi(const char *String)
{
    int Value = 0, Digit;
    int c;

    while ((c = *String++) != '\0') {

        if (c >= '0' && c <= '9')
            Digit = (c - '0');
        else
            break;

        Value = (Value * 10) + Digit;
    }

    return Value;
}

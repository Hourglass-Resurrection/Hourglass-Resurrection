//==========================================
// minicrt - Chris Benshoof 2009
// toupper from string.c (__ascii_toupper)
//==========================================
#include "libctiny.h"

extern "C" int __cdecl toupper(int c)
{
    if (c >= 'a' && c <= 'z') return (c - ('a' - 'A'));
    return c;
}
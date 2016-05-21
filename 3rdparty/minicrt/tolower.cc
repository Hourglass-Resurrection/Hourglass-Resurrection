//==========================================
// minicrt - Chris Benshoof 2009
// tolower from string.c (__ascii_tolower)
//==========================================
#include "libctiny.h"

extern "C" int __cdecl tolower(int c)
{
    if (c >= 'A' && c <= 'Z') return (c + ('a' - 'A'));
    return c;
}
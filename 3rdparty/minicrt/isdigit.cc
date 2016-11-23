//==========================================
// minicrt - Chris Benshoof 2009
// isdigit()
//==========================================
#include "libctiny.h"

extern "C" int __cdecl isdigit(int c)
{
    return (c >= '0' && c <= '9');
}
//==========================================
// minicrt - Chris Benshoof 2009
// srand() & rand(), from MSVCRT
//==========================================
#include "libctiny.h"

static unsigned long random_seed;

extern "C" void __cdecl srand(unsigned int seed)
{
    random_seed = (unsigned long)seed;
}

extern "C" int __cdecl rand() 
{
    return (((random_seed = random_seed * 214013L + 2531011L) >> 16) & 0x7fff);
}
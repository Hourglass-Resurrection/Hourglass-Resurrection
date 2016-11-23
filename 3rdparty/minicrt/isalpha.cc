//==========================================
// minicrt - Chris Benshoof 2009
// isalpha()
//==========================================
#include "libctiny.h"

// isalpha returns a nonzero value if c is within
// the ranges A – Z or a – z.
extern "C" int __cdecl isalpha(int c)
{
    return ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'));
}
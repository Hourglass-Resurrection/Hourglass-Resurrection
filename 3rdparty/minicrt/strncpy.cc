//==========================================
// minicrt - Chris Benshoof 2009
// strncpy, from MSVCRT
//==========================================
#include "libctiny.h"

extern "C" char * __cdecl strncpy (
        char * dest,
        const char * source,
        size_t count
        )
{
        char *start = dest;

        while (count && (*dest++ = *source++))    /* copy string */
                count--;

        if (count)                              /* pad out with zeroes */
                while (--count)
                        *dest++ = '\0';

        return(start);
}
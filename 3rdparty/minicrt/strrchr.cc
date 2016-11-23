//==========================================
// minicrt - Chris Benshoof 2009
// strrchr from MSVCRT
//==========================================
#include "libctiny.h"
#include <stdlib.h>

extern "C" char * __cdecl strrchr (const char * string, int ch)
{
    char *start = (char *)string;

    while (*string++)                       /* find end of string */
        ;
    /* search towards front */
    while (--string != start && *string != (char)ch)
        ;

    if (*string == (char)ch)                /* char found ? */
        return( (char *)string );

    return(NULL);
}
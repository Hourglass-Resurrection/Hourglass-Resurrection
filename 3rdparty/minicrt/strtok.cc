//==========================================
// minicrt - Chris Benshoof 2009
// strtok, from MSVCRT
//==========================================
#include "libctiny.h"
#include <stdlib.h>

static char * _token;

extern "C" char * __cdecl strtok (char * string, const char * control)
{
    char *str;                   // CWB: Changed this from unsigned
    const char *ctrl = control;  // CWB: Changed this from unsigned
    unsigned char map[32];
    int count;

    /* Clear control map */
    for (count = 0; count < 32; count++)
        map[count] = 0;

    /* Set bits in delimiter table */
    do {
        map[*ctrl >> 3] |= (1 << (*ctrl & 7));
    } while (*ctrl++);

    /* Initialize str */

    /* If string is NULL, set str to the saved
    * pointer (i.e., continue breaking tokens out of the string
    * from the last strtok call) */
    if (string)
        str = string;
    else
        str = _token;

    /* Find beginning of token (skip over leading delimiters). Note that
    * there is no token iff this loop sets str to point to the terminal
    * null (*str == '\0') */
    while ( (map[*str >> 3] & (1 << (*str & 7))) && *str )
        str++;

    string = str;

    /* Find the end of the token. If it is not the end of the string,
    * put a null there. */
    for ( ; *str ; str++ )
        if ( map[*str >> 3] & (1 << (*str & 7)) ) {
            *str++ = '\0';
            break;
        }

        /* Update nextoken (or the corresponding field in the per-thread data
        * structure */
        _token = str;

        /* Determine if a token has been found. */
        if ( string == str )
            return NULL;
        else
            return string;
}

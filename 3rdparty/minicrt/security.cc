#include "libctiny.h"

extern "C" {

#ifdef _WIN64
#define kDefaultSecurityCookie 0x2B992DDFA23249D6
#else  /* _WIN64 */
#define kDefaultSecurityCookie 0xBB40E64E
#endif  /* _WIN64 */

unsigned long __security_cookie = kDefaultSecurityCookie;

void __fastcall __security_check_cookie(unsigned long) {
    return;

    /* Immediately return if the local cookie is OK. */
    // if (cookie == __security_cookie)
    //    return;

    /* Report the failure */
    // report_failure();
}

};  // extern "C"

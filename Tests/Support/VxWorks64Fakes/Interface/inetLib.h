/* A test stand-in for the VxWorks 6.4 Internet address library header.
 *
 * Supplies the subset of inetLib.h that Platform/VxWorks64 calls, with the
 * prototype of the public API reference: the string is not const-qualified.
 */
#ifndef INETLIB_H
#define INETLIB_H

#include <netinet/in.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* A #define because it is one in the real header. The suppression lives here
 * for the reason semLib.h gives. */
/* NOLINTBEGIN(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum) */
#define INET_ADDR_LEN 18
    /* NOLINTEND(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum) */

    unsigned long inet_addr(char* inetString);
    void inet_ntoa_b(struct in_addr inetAddress, char* pString);

#ifdef __cplusplus
}
#endif

#endif /* INETLIB_H */

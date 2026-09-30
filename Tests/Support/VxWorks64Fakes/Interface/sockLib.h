/* A test stand-in for the VxWorks 6.4 socket library header.
 *
 * Supplies the subset of sockLib.h that Platform/VxWorks64 calls, with the
 * prototypes of the public API reference: sendto takes a non-const buffer and
 * int lengths.
 */
#ifndef SOCKLIB_H
#define SOCKLIB_H

#include <sys/socket.h>

#ifdef __cplusplus
extern "C"
{
#endif

    int socket(int domain, int type, int protocol);
    int sendto(int s, char* buf, int bufLen, int flags, struct sockaddr* to, int tolen);

#ifdef __cplusplus
}
#endif

#endif /* SOCKLIB_H */

/* A test stand-in for the VxWorks 6.4 socket library header.
 *
 * Supplies the subset of sockLib.h that Platform/VxWorks64 calls, with the
 * prototypes of the public API reference: sendto takes a non-const buffer and
 * int lengths, connectWithTimeout a non-const address, and setsockopt a char
 * pointer to the option's value.
 */
#ifndef SOCKLIB_H
#define SOCKLIB_H

#include <sys/socket.h>

struct timeval;

#ifdef __cplusplus
extern "C"
{
#endif

    int socket(int domain, int type, int protocol);
    int sendto(int s, char* buf, int bufLen, int flags, struct sockaddr* to, int tolen);
    STATUS connectWithTimeout(int sock, struct sockaddr* adrs, int adrsLen, struct timeval* timeVal);
    STATUS setsockopt(int s, int level, int optname, char* optval, int optlen);
    int send(int s, const char* buf, int bufLen, int flags);
    int recv(int s, char* buf, int bufLen, int flags);

#ifdef __cplusplus
}
#endif

#endif /* SOCKLIB_H */

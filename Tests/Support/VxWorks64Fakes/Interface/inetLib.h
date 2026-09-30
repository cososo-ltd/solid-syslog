/* A test stand-in for the VxWorks 6.4 Internet address library header.
 *
 * Supplies the subset of inetLib.h that Platform/VxWorks64 calls, with the
 * prototype of the public API reference: the string is not const-qualified.
 */
#ifndef INETLIB_H
#define INETLIB_H

#ifdef __cplusplus
extern "C"
{
#endif

    unsigned long inet_addr(char* inetString);

#ifdef __cplusplus
}
#endif

#endif /* INETLIB_H */

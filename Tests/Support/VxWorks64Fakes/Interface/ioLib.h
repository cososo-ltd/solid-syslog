/* A test stand-in for the VxWorks 6.4 I/O library header.
 *
 * Supplies the subset of ioLib.h that Platform/VxWorks64 calls. Relies on
 * vxWorks.h having been included first, as VxWorks does.
 */
#ifndef IOLIB_H
#define IOLIB_H

#ifdef __cplusplus
extern "C"
{
#endif

    STATUS close(int fd);

#ifdef __cplusplus
}
#endif

#endif /* IOLIB_H */

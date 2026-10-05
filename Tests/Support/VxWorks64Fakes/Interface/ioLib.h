/* A test stand-in for the VxWorks 6.4 I/O library header.
 *
 * Supplies the subset of ioLib.h that Platform/VxWorks64 calls. Relies on
 * vxWorks.h having been included first, as VxWorks does.
 *
 * The host C library, the test framework and the sanitizer runtimes call these
 * functions themselves, so each is renamed to the I/O fake's, as
 * ../ShadowInterface/README.md describes.
 */
#ifndef IOLIB_H
#define IOLIB_H

#define close VxWorks64IoFake_Close

#ifdef __cplusplus
extern "C"
{
#endif

    STATUS close(int fd);

#ifdef __cplusplus
}
#endif

#endif /* IOLIB_H */

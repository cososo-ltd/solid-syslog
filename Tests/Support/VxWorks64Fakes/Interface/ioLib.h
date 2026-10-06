/* A test stand-in for the VxWorks 6.4 I/O library header.
 *
 * Supplies the subset of ioLib.h that Platform/VxWorks64 calls, with the
 * kernel (not RTP) prototypes. Relies on vxWorks.h having been included first,
 * as VxWorks does. The real header reaches the O_ flags and SEEK_ constants
 * through the fcntl.h and unistd.h it includes; the stand-in defines them
 * itself rather than shadow those host headers.
 *
 * The host C library, the test framework and the sanitizer runtimes call these
 * functions themselves, so each is renamed to the I/O fake's, as
 * ../ShadowInterface/README.md describes.
 */
#ifndef IOLIB_H
#define IOLIB_H

#include <stddef.h>

/* NOLINTBEGIN(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum) */
#define O_RDONLY 0
#define O_RDWR 2
#define O_CREAT 0x0200

#define SEEK_SET 0
#define SEEK_END 2

#define FIOSYNC 21
#define FIOTRUNC 42
#define FIOCOMMITFS 56
/* NOLINTEND(cppcoreguidelines-macro-to-enum,modernize-macro-to-enum) */

#define open VxWorks64IoFake_Open
#define close VxWorks64IoFake_Close
#define read VxWorks64IoFake_Read
#define write VxWorks64IoFake_Write
#define lseek VxWorks64IoFake_Lseek
#define ioctl VxWorks64IoFake_Ioctl
#define remove VxWorks64IoFake_Remove

#ifdef __cplusplus
extern "C"
{
#endif

    typedef long off_t;

    int open(const char* name, int flags, int mode);
    STATUS close(int fd);
    int read(int fd, char* buffer, size_t maxbytes);
    int write(int fd, char* buffer, size_t nbytes);
    off_t lseek(int fd, off_t offset, int whence);
    int ioctl(int fd, int function, int arg);
    STATUS remove(const char* name);

#ifdef __cplusplus
}
#endif

#endif /* IOLIB_H */

#ifndef SOLIDSYSLOGSTREAM_H
#define SOLIDSYSLOGSTREAM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ExternC.h"

struct SolidSyslogAddress;

EXTERN_C_BEGIN

    /* Signed byte count. Models POSIX ssize_t using a standard-C type for portability
       to targets that lack <sys/types.h>'s ssize_t (notably MSVC). Semantics mirror
       POSIX recv(2): >=0 = bytes transferred, 0 = EOF on stream reads, <0 = error. */
    typedef intptr_t SolidSyslogSsize;

    struct SolidSyslogStream;

    bool             SolidSyslogStream_Open(struct SolidSyslogStream * stream, const struct SolidSyslogAddress* addr);
    bool             SolidSyslogStream_Send(struct SolidSyslogStream * stream, const void* buffer, size_t size);
    SolidSyslogSsize SolidSyslogStream_Read(struct SolidSyslogStream * stream, void* buffer, size_t size);
    void             SolidSyslogStream_Close(struct SolidSyslogStream * stream);

EXTERN_C_END

#endif /* SOLIDSYSLOGSTREAM_H */

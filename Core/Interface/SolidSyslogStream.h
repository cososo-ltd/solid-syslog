#ifndef SOLIDSYSLOGSTREAM_H
#define SOLIDSYSLOGSTREAM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ExternC.h"

struct SolidSyslogAddress;

EXTERN_C_BEGIN

    /* Signed byte count. Models POSIX ssize_t using a standard-C type for portability
       to targets that lack <sys/types.h>'s ssize_t (notably MSVC). */
    typedef intptr_t SolidSyslogSsize;

    struct SolidSyslogStream;

    /* Open a connection to addr. Returns true on success. On failure the underlying
       socket has already been closed internally; the caller can call Open again to retry. */
    bool SolidSyslogStream_Open(struct SolidSyslogStream * stream, const struct SolidSyslogAddress* addr);

    /* Non-blocking send. Returns true only when the kernel accepts the entire frame in
       a single call. Anything else — short write, EAGAIN/EWOULDBLOCK, EPIPE/ECONNRESET,
       any other error — closes the underlying socket internally and returns false; the
       caller must call Open before the next Send. Bounded so the service-thread drain
       rate stays insensitive to peer wedges or kernel send-buffer fill. */
    bool SolidSyslogStream_Send(struct SolidSyslogStream * stream, const void* buffer, size_t size);

    /* Non-blocking read.
         > 0  bytes transferred into buffer
         = 0  nothing available right now (would-block)
         < 0  EOF or error — the underlying socket has been closed internally; the
              caller must call Open before the next Send. */
    SolidSyslogSsize SolidSyslogStream_Read(struct SolidSyslogStream * stream, void* buffer, size_t size);

    /* Close the underlying socket. Idempotent. */
    void SolidSyslogStream_Close(struct SolidSyslogStream * stream);

EXTERN_C_END

#endif /* SOLIDSYSLOGSTREAM_H */

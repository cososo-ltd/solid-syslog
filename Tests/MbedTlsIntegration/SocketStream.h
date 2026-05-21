#ifndef SOCKETSTREAM_H
#define SOCKETSTREAM_H

#include "ExternC.h"

struct SolidSyslogStream;

EXTERN_C_BEGIN

    /* Minimal SolidSyslogStream wrapper over a raw socket fd. Open is a
     * no-op (the fd is supplied at Create time and assumed already-connected,
     * e.g. one end of a Unix socketpair). Send / Read use blocking write()
     * and read(); Close closes the fd. Used by the integration tests as the
     * client-side transport injected into SolidSyslogMbedTlsStream. */
    struct SolidSyslogStream* SocketStream_Create(int fd);
    void SocketStream_Destroy(struct SolidSyslogStream * self);

EXTERN_C_END

#endif /* SOCKETSTREAM_H */

#ifndef RECONNECTINGSOCKETSTREAM_H
#define RECONNECTINGSOCKETSTREAM_H

#include "ExternC.h"

struct SolidSyslogStream;

EXTERN_C_BEGIN

    /* A SolidSyslogStream wrapper that models a *reconnectable* transport for
     * the session-resumption integration tests. Unlike SocketStream (one fixed
     * fd, no-op Open), each Open consumes the next fd from a caller-supplied
     * queue, so the library's real Close -> Open fail-fast reconnect lands on a
     * fresh connection — exactly what session resumption needs to be observed
     * end-to-end. The fds are pre-connected socketpair ends (no TCP ports), so
     * the harness stays deterministic. Ownership of the fds transfers to the
     * stream: Close closes the current one, Destroy closes any still open. */
    struct SolidSyslogStream* ReconnectingSocketStream_Create(const int* fds, int count);
    void ReconnectingSocketStream_Destroy(struct SolidSyslogStream * self);

EXTERN_C_END

#endif /* RECONNECTINGSOCKETSTREAM_H */

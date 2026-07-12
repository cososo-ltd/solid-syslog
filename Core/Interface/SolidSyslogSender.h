#ifndef SOLIDSYSLOGSENDER_H
#define SOLIDSYSLOGSENDER_H

#include "ExternC.h"
#include <stdbool.h>
#include <stddef.h>

EXTERN_C_BEGIN

    struct SolidSyslogSender;

    /** Deliver one framed message. Returns true when it is on the wire (the caller
     *  may then drop it), false when it should be retried later. May connect on
     *  first use, which can block on the transport. @p buffer is read only during the call. */
    bool SolidSyslogSender_Send(struct SolidSyslogSender * sender, const void* buffer, size_t size);

    /** Drop the connection; the next Send reconnects. Idempotent. */
    void SolidSyslogSender_Disconnect(struct SolidSyslogSender * sender);

EXTERN_C_END

#endif /* SOLIDSYSLOGSENDER_H */

#ifndef SOLIDSYSLOGSENDERDEFINITION_H
#define SOLIDSYSLOGSENDERDEFINITION_H

#include <stdbool.h>
#include <stddef.h>

#include "ExternC.h"

EXTERN_C_BEGIN

    /** The contract an implementer fills in. The library composes one sender per
     *  destination and calls it from the servicing pass; a sender is not a
     *  process-singleton (dual-SIEM and failover stacks hold several). */
    struct SolidSyslogSender
    {
        /** Deliver one fully-framed message, @p buffer[0..size). Return true only
         *  once it is on the wire (or genuinely unrecoverable and safe to drop):
         *  the Service loop marks the record sent and moves on. Return false to
         *  keep the record in the Store for a later retry, which is what a slow or
         *  down destination reports. May connect lazily, which can block on the
         *  transport; @p buffer is read during the call and need not outlive it. Called
         *  on the servicing thread, so it need not be reentrant. */
        bool (*Send)(struct SolidSyslogSender* base, const void* buffer, size_t size);
        /** Drop the underlying connection, leaving the sender reusable: the next
         *  Send reconnects. Must be idempotent (safe when already disconnected).
         *  The library calls this to reset a stale connection, not on every send
         *  failure, so it is not on the hot path. */
        void (*Disconnect)(struct SolidSyslogSender* base);
    };

EXTERN_C_END

#endif /* SOLIDSYSLOGSENDERDEFINITION_H */

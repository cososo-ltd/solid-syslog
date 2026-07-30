/** @file
 *  The Datagram vtable (Open / SendTo / MaxPayload / Close) — the unconnected
 *  (UDP) transport contract an implementor fills in (the Datagram extension
 *  point). */
#ifndef SOLIDSYSLOGDATAGRAMDEFINITION_H
#define SOLIDSYSLOGDATAGRAMDEFINITION_H

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogDatagram.h"
#include "SolidSyslogExternC.h"

struct SolidSyslogAddress;

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Vtable an unconnected-datagram (UDP) transport implements. Each method
     *  receives the same struct as its first argument, so an implementation
     *  embeds this as its first member and downcasts. Per-method contract is on
     *  the SolidSyslogDatagram_* wrappers in SolidSyslogDatagram.h. */
    struct SolidSyslogDatagram
    {
        bool (*Open)(struct SolidSyslogDatagram* base);
        enum SolidSyslogDatagramSendResult (*SendTo)(
            struct SolidSyslogDatagram* base,
            const void* buffer,
            size_t size,
            const struct SolidSyslogAddress* addr
        );
        size_t (*MaxPayload)(struct SolidSyslogDatagram* base);
        void (*Close)(struct SolidSyslogDatagram* base);
    };

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGDATAGRAMDEFINITION_H */

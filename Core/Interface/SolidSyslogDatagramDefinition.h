/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The Datagram vtable (Open / SendTo / MaxPayload / Close) - the unconnected
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
     *  embeds this as its first member and downcasts.
     *
     *  SolidSyslogDatagram.h states what a caller may expect; what an implementor
     *  must guarantee is here, because the two are not the same question:
     *
     *  - **Report MaxPayload for the path in use, and never guess high.** The
     *    value bounds what the caller will retry, so an optimistic answer costs
     *    the record. Where the stack cannot report a path MTU, answer
     *    SOLIDSYSLOG_UDP_IPV6_SAFE_PAYLOAD rather than something larger. The
     *    path meant is the destination currently being sent to; an
     *    implementation serving several at once answers low enough for all of
     *    them.
     *  - **Return OVERSIZE where the platform can distinguish it**, so the caller
     *    can trim and retry rather than treating the record as undeliverable.
     *    Collapsing it into FAILED is permitted for a stack that cannot tell the
     *    difference, and costs recovery: see the note on SendTo.
     *  - **SENT means the record has been handed to the network**, and licenses
     *    the caller to drop it. Do not return SENT for a datagram still queued
     *    behind something that may fail.
     *  - **Open and Close are the whole lifecycle.** Close is idempotent and safe
     *    on an unopened datagram, because the caller's failure paths call it. */
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

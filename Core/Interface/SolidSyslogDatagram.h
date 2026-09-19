/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The datagram role: connectionless send of one message to an address (Open /
 *  SendTo / Close), with a path-MTU hint (MaxPayload). These calls dispatch to
 *  the injected datagram's vtable, so behaviour is that datagram's. */
#ifndef SOLIDSYSLOGDATAGRAM_H
#define SOLIDSYSLOGDATAGRAM_H

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogExternC.h"

struct SolidSyslogAddress;

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogDatagram;

    /* Distinct outcomes of SendTo. Oversize is reserved for the EMSGSIZE
     * recovery path (S12.12) - implementations that cannot detect oversize
     * collapse it into Failed. */
    enum SolidSyslogDatagramSendResult
    {
        SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT, /**< Handed to the network; the sender may drop the record. */
        SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE, /**< Rejected for size; retriable trimmed to MaxPayload. */
        SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED /**< Not sent; keep the record and retry later. */
    };

    /** Ready the transport for SendTo (acquire the socket). Failure leaves the
     *  datagram unopened; the sender retries on the next servicing pass. */
    bool SolidSyslogDatagram_Open(struct SolidSyslogDatagram * datagram);

    /** Send @p size bytes of @p buffer to @p addr as one datagram. @p addr is
     *  borrowed for the call only. A SENT result licenses the sender to drop the
     *  record, so a Null datagram returns SENT to discard silently rather than
     *  let undeliverables accumulate in the store.
     *
     *  @retval SOLIDSYSLOG_DATAGRAM_SEND_RESULT_SENT     Delivered to the network.
     *  @retval SOLIDSYSLOG_DATAGRAM_SEND_RESULT_OVERSIZE Too large for the path;
     *                                                    the sender trims to MaxPayload and retries.
     *  @retval SOLIDSYSLOG_DATAGRAM_SEND_RESULT_FAILED   Transient failure; the record is kept.
     *
     *  FAILED is transient by design: its usual causes - an unreachable
     *  collector, a wrong address or port, a stack not yet up - are resolved
     *  outside the library, and holding the record until they are is what an
     *  audit trail wants. Size is the one cause the record itself carries.
     *
     *  The trim is reactive: the sender offers the record at full size and
     *  calls SolidSyslogDatagram_MaxPayload only once a send has not
     *  succeeded, so nothing is asked of an implementation while sends are
     *  working. An implementation that collapses OVERSIZE into FAILED is still
     *  recovered from: the sender compares the record against MaxPayload and
     *  trims it if it would not have fitted, leaving a FAILED on a record that
     *  does fit to be retried whole. An implementation answering zero says it
     *  cannot report what the path carries, so a failure is never read as
     *  oversize on its word. An explicit OVERSIZE alongside it is a different
     *  statement - the record is too big and the implementation will not say
     *  what would fit - and the record is discarded rather than offered for
     *  ever. */
    enum SolidSyslogDatagramSendResult SolidSyslogDatagram_SendTo(
        struct SolidSyslogDatagram * datagram,
        const void* buffer,
        size_t size,
        const struct SolidSyslogAddress* addr
    );

    /** Largest datagram payload the current path is known to accept, used to
     *  trim a record a send did not carry. Before a path MTU is known, or
     *  where the stack cannot report one, an implementation falls back to
     *  SolidSyslogUdpPayload_UnknownPath for the destination's address
     *  family. */
    size_t SolidSyslogDatagram_MaxPayload(struct SolidSyslogDatagram * datagram);

    /** Release the transport acquired by Open. Idempotent; safe on an unopened
     *  datagram. */
    void SolidSyslogDatagram_Close(struct SolidSyslogDatagram * datagram);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGDATAGRAM_H */

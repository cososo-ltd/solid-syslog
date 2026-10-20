/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  An in-memory ring Buffer that decouples Log (enqueue) from Service (drain),
 *  backed entirely by caller-supplied storage - no allocation of its own.
 *
 *  Records are framed with a uint16 length prefix and stored back-to-back. A
 *  record is never split across the ring's end: one that would straddle the
 *  wrap point is written whole to the front, leaving a gap that the reader skips
 *  via a wrap marker. A full ring applies the injected discard policy: Oldest
 *  evicts records from the head to make room, Newest and Halt refuse the record
 *  being written. Either loss is reported as SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST,
 *  once per run, with a detail saying which.
 *  A record larger than SOLIDSYSLOG_MAX_MESSAGE_SIZE, which SolidSyslog_Log never
 *  formats, is rejected outright and reported as SOLIDSYSLOG_CAT_BAD_ARGUMENT.
 *  A Read whose buffer is too small for the head record leaves the record in
 *  place and reports nothing delivered.
 *
 *  Every enqueue and drain is bracketed by the injected mutex, so the two sides
 *  are safe on separate tasks; inject SolidSyslogNullMutex_Get() for single-task
 *  use where the lock is pure overhead. */
#ifndef SOLIDSYSLOGCIRCULARBUFFER_H
#define SOLIDSYSLOGCIRCULARBUFFER_H

#include "SolidSyslogExternC.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogDiscardPolicy.h"
#include "SolidSyslogTunables.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogBuffer;
    struct SolidSyslogMutex;

    enum
    {
        /** Per-record framing overhead: a uint16 length prefix. */
        SOLIDSYSLOG_CIRCULAR_BUFFER_HEADER_BYTES = sizeof(uint16_t)
    };

    /** Ring bytes to hold @p maxMessages worst-case records (each a full
     *  SOLIDSYSLOG_MAX_MESSAGE_SIZE payload plus its length prefix). Smaller
     *  messages pack tighter, so this is a capacity floor, not an exact fit. */
#define SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(maxMessages) \
    ((maxMessages) * (SOLIDSYSLOG_MAX_MESSAGE_SIZE + SOLIDSYSLOG_CIRCULAR_BUFFER_HEADER_BYTES))

    /** In-memory ring Buffer: Log enqueues, Service drains, decoupling the two.
     *  The caller supplies the backing @p ring (@p ringBytes wide, sized via
     *  SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES) and a @p mutex making the enqueue
     *  and drain sides mutually safe when they run on different tasks; inject
     *  SolidSyslogNullMutex_Get() for single-task use. Both must outlive the
     *  buffer. Records are length-prefixed and never split across the wrap point
     *  (a record that would straddle the end wraps whole to the front).
     *
     *  @p getDiscardPolicy is asked, with @p discardPolicyContext, on a write
     *  the full ring cannot take, from the task that logs; give it the function
     *  the logger has, so the pipeline applies one policy. NULL applies Oldest.
     *  Under Oldest the ring drops records from its head until the new one
     *  fits, and refuses without evicting a record no empty ring could hold;
     *  Newest and Halt refuse the record being written. An exhausted pool falls
     *  back to the shared NullBuffer. */
    struct SolidSyslogBuffer* SolidSyslogCircularBuffer_Create(
        struct SolidSyslogMutex * mutex,
        uint8_t* ring,
        size_t ringBytes,
        SolidSyslogDiscardPolicyFunction getDiscardPolicy,
        void* discardPolicyContext
    );
    /** Release the pool slot; does not free the caller-supplied ring or mutex. */
    void SolidSyslogCircularBuffer_Destroy(struct SolidSyslogBuffer * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGCIRCULARBUFFER_H */

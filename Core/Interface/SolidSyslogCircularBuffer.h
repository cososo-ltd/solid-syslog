/** @file
 *  An in-memory ring Buffer that decouples Log (enqueue) from Service (drain),
 *  backed entirely by caller-supplied storage — no allocation of its own.
 *
 *  Records are framed with a uint16 length prefix and stored back-to-back. A
 *  record is never split across the ring's end: one that would straddle the
 *  wrap point is written whole to the front, leaving a gap that the reader skips
 *  via a wrap marker. On a full ring the newest record is dropped (the write is
 *  simply refused) rather than overwriting unsent data, so the oldest queued
 *  records survive. A record larger than SOLIDSYSLOG_MAX_MESSAGE_SIZE is
 *  rejected outright, and a Read whose buffer is too small for the head record
 *  leaves the record in place and reports nothing delivered.
 *
 *  Every enqueue and drain is bracketed by the injected mutex, so the two sides
 *  are safe on separate tasks; inject SolidSyslogNullMutex_Get() for single-task
 *  use where the lock is pure overhead. */
#ifndef SOLIDSYSLOGCIRCULARBUFFER_H
#define SOLIDSYSLOGCIRCULARBUFFER_H

#include "ExternC.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogTunables.h"

EXTERN_C_BEGIN

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
     *  (a record that would straddle the end wraps whole to the front); on a full
     *  ring the newest record is dropped. An exhausted pool falls back to the
     *  shared NullBuffer. */
    struct SolidSyslogBuffer* SolidSyslogCircularBuffer_Create(
        struct SolidSyslogMutex * mutex,
        uint8_t* ring,
        size_t ringBytes
    );
    /** Release the pool slot; does not free the caller-supplied ring or mutex. */
    void SolidSyslogCircularBuffer_Destroy(struct SolidSyslogBuffer * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGCIRCULARBUFFER_H */

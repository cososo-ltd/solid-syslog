/** @file
 *  The buffer role: producer Write / consumer Read that decouples
 *  SolidSyslog_Log from Service. These calls dispatch to the injected buffer's
 *  vtable, so behaviour — notably whether Write blocks or returns at once — is
 *  that buffer's. */
#ifndef SOLIDSYSLOGBUFFER_H
#define SOLIDSYSLOGBUFFER_H

#include "ExternC.h"
#include <stdbool.h>
#include <stddef.h>

EXTERN_C_BEGIN

    struct SolidSyslogBuffer;

    /** Producer side: hand one formatted record to the buffer. @p data is copied
     *  as needed and need not outlive the call. There is no back-pressure signal:
     *  whether this blocks (PassthroughBuffer sends inline) or returns at once
     *  (CircularBuffer enqueues) is the buffer's choice, and a record that does
     *  not fit is dropped silently. */
    void SolidSyslogBuffer_Write(struct SolidSyslogBuffer * buffer, const void* data, size_t size);

    /** Consumer side, called by SolidSyslog_Service to drain the buffer. Returns
     *  true with one record copied into @p data (up to @p maxSize) and its length
     *  in @p bytesRead; returns false when nothing was delivered, which the drain
     *  loop reads as "empty, stop". @p bytesRead is always set (0 on false). A
     *  head record too large for @p maxSize also returns false and is left
     *  un-dequeued (the drain stalls at it) — this cannot arise under correct
     *  configuration, so an implementation reports it via SolidSyslog_Error under
     *  the buffer-backend-failed category rather than failing silently. */
    bool SolidSyslogBuffer_Read(struct SolidSyslogBuffer * buffer, void* data, size_t maxSize, size_t* bytesRead);

EXTERN_C_END

#endif /* SOLIDSYSLOGBUFFER_H */

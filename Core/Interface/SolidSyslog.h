#ifndef SOLIDSYSLOG_H
#define SOLIDSYSLOG_H

#include "ExternC.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogServiceStatus.h"

#include <stddef.h>

EXTERN_C_BEGIN

    struct SolidSyslog;
    struct SolidSyslogStructuredData;

    /** One event to log. The strings are read (formatted into the message)
     *  during the Log call and need not outlive it. */
    struct SolidSyslogMessage
    {
        enum SolidSyslogFacility Facility;
        enum SolidSyslogSeverity Severity;
        const char* MessageId; /**< RFC 5424 MSGID: a short event-type tag (e.g. "BOOT"), not a unique id. */
        const char* Msg; /**< RFC 5424 MSG: the free-form event text. */
    };

    /** Format @p message and hand it to the configured Buffer. Whether this
     *  blocks depends on that Buffer: a PassthroughBuffer sends inline; a
     *  CircularBuffer enqueues and returns, leaving delivery to
     *  SolidSyslog_Service. Concurrent calls are safe only if the configured
     *  Buffer is thread-safe (e.g. a CircularBuffer with a real Mutex). A NULL handle or
     *  message is reported via SolidSyslog_Error and otherwise ignored. */
    void SolidSyslog_Log(struct SolidSyslog * handle, const struct SolidSyslogMessage* message);

    /** As SolidSyslog_Log, but also attaches @p sd[0..sdCount) as caller-built
     *  SD-ELEMENTs to this one message, emitted after the per-instance SDs
     *  registered at Create. The SD objects need only live for the duration of
     *  the call. (SolidSyslog_Log is this with sd = NULL, sdCount = 0.) */
    void SolidSyslog_LogWithSd(
        struct SolidSyslog * handle,
        const struct SolidSyslogMessage* message,
        struct SolidSyslogStructuredData** sd,
        size_t sdCount
    );

    /** Run one servicing pass (drain the buffer into the store, then attempt one
     *  send from the store) and return an advisory hint for driving an
     *  event-driven loop. The hint is never load-bearing: each call re-derives
     *  its state from the live buffer and store, so a missed or spurious wake is
     *  safe and a fixed-delay poll loop may ignore the return. A NULL handle is
     *  reported and returns IDLE.
     *
     *  @retval SOLIDSYSLOG_SERVICE_IDLE    Nothing to do anywhere; wait for the next Log.
     *  @retval SOLIDSYSLOG_SERVICE_READY   Work moved this pass, or the store still has
     *                                      unsent after a send; loop again.
     *  @retval SOLIDSYSLOG_SERVICE_BLOCKED The buffer was idle and only the sender is stuck
     *                                      (a send was attempted and failed); back off, a
     *                                      Log will wake it.
     *  @retval SOLIDSYSLOG_SERVICE_HALTED  The store is halted (only under
     *                                      SolidSyslogDiscardPolicy_Halt); alarm / slow heartbeat.
     *
     *  Buffer drain out-ranks a send failure: a down sender never demotes the
     *  loop out of "keep draining" while the buffer still has records to move. */
    enum SolidSyslogServiceStatus SolidSyslog_Service(struct SolidSyslog * handle);

EXTERN_C_END

#endif /* SOLIDSYSLOG_H */

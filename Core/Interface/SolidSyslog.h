#ifndef SOLIDSYSLOG_H
#define SOLIDSYSLOG_H

#include "ExternC.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogServiceStatus.h"

#include <stddef.h>

EXTERN_C_BEGIN

    struct SolidSyslog;
    struct SolidSyslogStructuredData;

    struct SolidSyslogMessage
    {
        enum SolidSyslogFacility Facility;
        enum SolidSyslogSeverity Severity;
        const char* MessageId;
        const char* Msg;
    };

    void SolidSyslog_Log(struct SolidSyslog * handle, const struct SolidSyslogMessage* message);

    /* As SolidSyslog_Log, but attaches sd[0..sdCount) as caller-built SD-ELEMENTs to this one
       message, emitted after the per-instance SDs registered at Create. The SD objects need only
       live for the duration of the call. SolidSyslog_Log is exactly this with sd = NULL, count = 0. */
    void SolidSyslog_LogWithSd(
        struct SolidSyslog * handle,
        const struct SolidSyslogMessage* message,
        struct SolidSyslogStructuredData** sd,
        size_t sdCount
    );

    /* Runs one servicing pass — drain the buffer into the store, then attempt one send from the
       store — and returns an advisory hint a host can use to drive an event-driven loop. The hint
       is never load-bearing: each call re-derives its state from the live buffer and store, so a
       missed or spurious wake is safe and a fixed-delay poll loop may ignore the return.

         SOLIDSYSLOG_SERVICE_IDLE    — nothing to do anywhere; also returned for a NULL handle.
                                       Host: wait for the next Log.
         SOLIDSYSLOG_SERVICE_READY   — work is ready now: this pass moved >=1 buffered record, or the
                                       store still has unsent after a successful send. Host: loop again.
         SOLIDSYSLOG_SERVICE_BLOCKED — the buffer was idle and the only stuck thing is the sender (a
                                       send was attempted and failed). Host: back off, wakeable by a Log.
         SOLIDSYSLOG_SERVICE_HALTED  — the store is halted, so nothing can be processed. Only reachable
                                       when the store is configured to halt when full
                                       (SolidSyslogDiscardPolicy_Halt). Host: alarm / slow heartbeat.

       Buffer drain out-ranks a send failure: a down sender can never demote the loop out of "keep
       draining the buffer" while the buffer still has anything to move. */
    enum SolidSyslogServiceStatus SolidSyslog_Service(struct SolidSyslog * handle);

EXTERN_C_END

#endif /* SOLIDSYSLOG_H */

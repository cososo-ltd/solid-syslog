/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The setup-time wiring: the SolidSyslogConfig struct and SolidSyslog_Create /
 *  SolidSyslog_Destroy that build and release a logger from it. */
#ifndef SOLIDSYSLOGCONFIG_H
#define SOLIDSYSLOGCONFIG_H

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogDiscardPolicy.h"
#include "SolidSyslogHeaderFieldFunction.h"
#include "SolidSyslogTimestamp.h"
#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslog;
    struct SolidSyslogBuffer;
    struct SolidSyslogHeaderField;
    struct SolidSyslogScratch;
    struct SolidSyslogSender;
    struct SolidSyslogStore;
    struct SolidSyslogStructuredData;

    /** Called once when the logger halts: a queue has refused a record while the
     *  discard policy answers Halt. It runs on the task that calls
     *  SolidSyslog_Service, after the refusal has been reported, so a callback
     *  that stops the device and never returns loses nothing the error handler
     *  would have seen. */
    typedef void (*SolidSyslogHaltCallback)(void* context);

    /** Answers whether the logger is to report one of the library's own events
     *  as a syslog record of its own. */
    typedef bool (*SolidSyslogEventReportingFunction)(void* context);

    /** Wiring for SolidSyslog_Create. NULL is safe for any field: Buffer,
     *  Sender, and Store fall back to their Null object and report an error
     *  (a no-op unless an error handler is installed; inject the Null* object
     *  instead to stay silent), while the callbacks fall back to nil defaults
     *  and Scratch to the stack scratch, with no error. Whatever you do provide must outlive the created handle;
     *  the config struct itself is only read during Create and may be transient. */
    struct SolidSyslogConfig
    {
        struct SolidSyslogBuffer* Buffer; /**< Also sets Log's blocking behaviour (inline vs enqueued). */
        struct SolidSyslogSender* Sender;
        SolidSyslogClockFunction Clock; /**< NULL leaves timestamps at the RFC 5424 nil value. */
        SolidSyslogHeaderFieldFunction GetHostname; /**< RFC 5424 HOSTNAME; NULL emits "-". */
        void* GetHostnameContext; /**< Passed back to the callback unchanged. */
        SolidSyslogHeaderFieldFunction GetAppName; /**< RFC 5424 APP-NAME; NULL emits "-". */
        void* GetAppNameContext; /**< Passed back to the callback unchanged. */
        SolidSyslogHeaderFieldFunction GetProcessId; /**< RFC 5424 PROCID; NULL emits "-". */
        void* GetProcessIdContext; /**< Passed back to the callback unchanged. */
        struct SolidSyslogStore* Store; /**< NULL (or SolidSyslogNullStore_Get()) means no store-and-forward. */
        struct SolidSyslogStructuredData** Sd; /**< @optional per-instance SD-ELEMENTs, before any per-message SD. */
        size_t SdCount;
        /** Asked, with DiscardPolicyContext, when Service has to decide what a
         *  refused record means: under Halt the logger stops moving records,
         *  and under Oldest it drops a record from the buffer that it holds
         *  for the sender once the buffer has filled behind it; a report of
         *  one of the library's own events keeps its place. Give the store
         *  and the buffer the same function at their own Create; the logger's
         *  answer decides those two things, the store's and the buffer's
         *  whether each evicts or refuses, and nothing checks that they agree.
         *  NULL applies Oldest. */
        SolidSyslogDiscardPolicyFunction GetDiscardPolicy;
        void* DiscardPolicyContext;
        SolidSyslogHaltCallback OnHalt; /**< @optional nothing is called when NULL. */
        void* HaltContext; /**< Passed back to OnHalt unchanged. */
        /** Where Log formats a message: on the stack of the task that logs
         *  (SolidSyslogStackScratch_Get(), and what NULL means) or in one buffer
         *  shared under a mutex (SolidSyslogSharedScratch_Create). */
        struct SolidSyslogScratch* Scratch;
        /** Asked, with EventReportingContext, on the task that calls
         *  SolidSyslog_Service, each time Service is about to report one of the
         *  library's own events: the logger holds no record and an event is
         *  waiting. False leaves that event, and any the pass had still to
         *  report, waiting for a pass when the answer is true, and lets this
         *  pass return SOLIDSYSLOG_SERVICE_IDLE. A loop waiting on IDLE is not
         *  woken when the answer turns true; the next Log, or a poll, makes the
         *  report. A report already made is not held back. Because it is asked
         *  rather than read once at Create, a device can turn reporting off and
         *  on while it runs. NULL means on. */
        SolidSyslogEventReportingFunction IsEventReportingOn;
        void* EventReportingContext; /**< Passed back to IsEventReportingOn unchanged. */
    };

    /** Create a logger from @p config. Never returns NULL: on a NULL config or
     *  an exhausted instance pool it reports via SolidSyslog_Error and returns a
     *  shared null instance whose Log/Service silently drop, so callers need not
     *  null-check the result. */
    struct SolidSyslog* SolidSyslog_Create(const struct SolidSyslogConfig* config);

    /** Release the logger's pool slot, and forget every library event it has
     *  not yet reported, so a logger created afterwards reports only what
     *  happens after it. Does not free the injected Buffer, Sender, Store, or
     *  Sd objects; the caller owns those. A NULL, foreign, or already-destroyed
     *  handle is reported via SolidSyslog_Error and ignored. */
    void SolidSyslog_Destroy(struct SolidSyslog * handle);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGCONFIG_H */

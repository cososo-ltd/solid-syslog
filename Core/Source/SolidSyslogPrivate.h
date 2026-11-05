/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGPRIVATE_H
#define SOLIDSYSLOGPRIVATE_H

#include <stddef.h>

#include "SolidSyslogConfig.h"
#include "SolidSyslogDiscardPolicy.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrors.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogMessageFormatter.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTimestamp.h"

struct SolidSyslogBuffer;
struct SolidSyslogConfig;
struct SolidSyslogHeaderField;
struct SolidSyslogScratch;
struct SolidSyslogSender;
struct SolidSyslogStore;

struct SolidSyslog
{
    struct SolidSyslogBuffer* Buffer;
    struct SolidSyslogSender* Sender;
    struct SolidSyslogStore* Store;
    struct SolidSyslogScratch* Scratch;
    struct SolidSyslogMessageFormatterContext Format;
    /* The one record Service works on at a time: taken from the buffer, or read
     * from the store to send. It outlives a Service pass so that a record no
     * store will take can be kept for the next; HoldsRecord is true while one is
     * kept that way, and Service reads neither the buffer nor the store until it
     * has gone. Service alone puts a record in the slot, from the one task that
     * calls it.
     *
     * The storage is the pool entry's, not the instance's: Create points
     * RecordStorage at it, and Record at the formatter's bytes within it, so
     * Service can format its own report into the slot it sends from. Cleanup
     * takes it away, leaving both NULL and RecordCapacity 0. Destroy and the
     * fallback SolidSyslog_Create returns both go through Cleanup, so the
     * fallback carries no slot. Cleanup also installs the Null buffer and
     * store, which deliver no record, so a NULL Record never meets a Read that
     * could write into it. */
    SolidSyslogFormatterStorage* RecordStorage;
    char* Record;
    size_t RecordCapacity;
    size_t RecordLength;
    bool HoldsRecord;
    /* Whether the record held is a report Service made of one of the
     * library's own events, rather than a record from the buffer. */
    bool HeldRecordIsReport;
    /* A report stands for a run of held records dropped because the buffer
     * filled behind them; a record from the buffer dealt with ends the run. */
    bool HeldRecordLossReported;
    SolidSyslogDiscardPolicyFunction GetDiscardPolicy;
    void* DiscardPolicyContext;
    SolidSyslogHaltCallback OnHalt;
    void* HaltContext;
    SolidSyslogEventReportingFunction IsEventReportingOn;
    void* EventReportingContext;
    /* Whether the switch turned a report down this pass; each pass starts
     * without. */
    bool ReportTurnedDown;
    /* Set while the policy answers Halt, when a retaining store refuses a
     * record or the buffer has filled behind the record Service holds; Service
     * then moves nothing until the answer changes. */
    bool Halted;
};

void SolidSyslog_Initialise(
    struct SolidSyslog* self,
    const struct SolidSyslogConfig* config,
    SolidSyslogFormatterStorage* recordStorage,
    size_t recordCapacity
);
void SolidSyslog_Cleanup(struct SolidSyslog* self);

static inline void SolidSyslog_Report(enum SolidSyslogSeverity severity, uint16_t category, enum SolidSyslogErrors code)
{
    SolidSyslog_Error(severity, &SolidSyslogErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGPRIVATE_H */

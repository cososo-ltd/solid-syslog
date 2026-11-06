/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslog.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogBuffer.h"
#include "SolidSyslogBufferCategories.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogDiscardPolicy.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogErrors.h"
#include "SolidSyslogEventSd.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogLoggerCategories.h"
#include "SolidSyslogMessageFormatter.h"
#include "SolidSyslogNullBuffer.h"
#include "SolidSyslogNullSender.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogPendingEvents.h"
#include "SolidSyslogPrivate.h"
#include "SolidSyslogHeaderFieldFunction.h"
#include "SolidSyslogScratch.h"
#include "SolidSyslogSender.h"
#include "SolidSyslogStackScratch.h"
#include "SolidSyslogStore.h"
#include "SolidSyslogStoreDefinition.h"
#include "SolidSyslogTimestamp.h"
#include "SolidSyslogTunables.h"

const struct SolidSyslogErrorSource SolidSyslogErrorSource = {"SolidSyslog", SolidSyslogErrors_Name};

struct SolidSyslogBuffer;
struct SolidSyslogHeaderField;
struct SolidSyslogScratch;
struct SolidSyslogSender;
struct SolidSyslogStore;
struct SolidSyslogStructuredData;

/* What Log carries into the scratch: the continuation formats from it. */
struct LogRequest
{
    struct SolidSyslog* Handle;
    const struct SolidSyslogMessage* Message;
    struct SolidSyslogStructuredData** Sd;
    size_t SdCount;
};

static inline bool SolidSyslog_DrainBufferIntoStore(struct SolidSyslog* self);
static inline void SolidSyslog_ApplyPolicyToHeldRecordIfBufferFull(struct SolidSyslog* self);
static inline void SolidSyslog_DropHeldRecord(struct SolidSyslog* self);
static bool SolidSyslog_HoldNextRecord(struct SolidSyslog* self);
static inline bool SolidSyslog_ReportPendingEvents(struct SolidSyslog* self);
static inline bool SolidSyslog_HasRecordSlot(const struct SolidSyslog* self);
static inline bool SolidSyslog_ReportIfPending(struct SolidSyslog* self, size_t index);
static inline void SolidSyslog_HoldPendingReport(struct SolidSyslog* self, size_t index);
static inline void SolidSyslog_FormatReport(struct SolidSyslog* self, const struct SolidSyslogErrorEvent* event);
static bool SolidSyslog_DealWithHeldRecord(struct SolidSyslog* self);
static inline bool SolidSyslog_IsBufferRecordDealtWith(const struct SolidSyslog* self);
static inline bool SolidSyslog_StoreOrSenderTakesHeldRecord(struct SolidSyslog* self);
static inline bool SolidSyslog_StoreDealsWithHeldRecord(struct SolidSyslog* self);
static inline void SolidSyslog_HaltIfPolicyHalts(struct SolidSyslog* self);
static inline void SolidSyslog_Halt(struct SolidSyslog* self);
static inline bool SolidSyslog_SendOneFromStore(struct SolidSyslog* self);
static inline bool SolidSyslog_TakeNextFromStore(struct SolidSyslog* self);
static inline bool SolidSyslog_IsBlocked(struct SolidSyslog* self, bool storeBlocked, bool drained);
static inline bool SolidSyslog_IsReportWaiting(const struct SolidSyslog* self);
static void SolidSyslog_DoLog(
    struct SolidSyslog* handle,
    const struct SolidSyslogMessage* message,
    struct SolidSyslogStructuredData** sd,
    size_t sdCount
);
static void SolidSyslog_FormatIntoScratch(SolidSyslogFormatterStorage* storage, size_t messageBytes, void* context);
static void SolidSyslog_InstallScratch(struct SolidSyslog* self, struct SolidSyslogScratch* configured);
static void SolidSyslog_InstallAppName(
    struct SolidSyslog* self,
    SolidSyslogHeaderFieldFunction configured,
    void* context
);
static void SolidSyslog_InstallBuffer(struct SolidSyslog* self, struct SolidSyslogBuffer* configured);
static void SolidSyslog_InstallClock(struct SolidSyslog* self, SolidSyslogClockFunction configured);
static void SolidSyslog_InstallDiscardPolicy(
    struct SolidSyslog* self,
    SolidSyslogDiscardPolicyFunction configured,
    void* context
);
static void SolidSyslog_InstallHostname(
    struct SolidSyslog* self,
    SolidSyslogHeaderFieldFunction configured,
    void* context
);
static void SolidSyslog_InstallProcessId(
    struct SolidSyslog* self,
    SolidSyslogHeaderFieldFunction configured,
    void* context
);
static void SolidSyslog_InstallSender(struct SolidSyslog* self, struct SolidSyslogSender* configured);
static void SolidSyslog_InstallStore(struct SolidSyslog* self, struct SolidSyslogStore* configured);
static void SolidSyslog_InstallStructuredData(
    struct SolidSyslog* self,
    struct SolidSyslogStructuredData** configured,
    size_t count
);
static inline void SolidSyslog_ResumeIfPolicyChanged(struct SolidSyslog* self);
static inline bool SolidSyslog_IsServiceEnabled(const struct SolidSyslog* self);
static inline bool SolidSyslog_PolicyHalts(struct SolidSyslog* self);
static void SolidSyslog_NullClock(struct SolidSyslogTimestamp* ts);
static void SolidSyslog_NullHeaderField(struct SolidSyslogHeaderField* field, void* context);
static void SolidSyslog_NullHalt(void* context);
static void SolidSyslog_InstallHalt(struct SolidSyslog* self, SolidSyslogHaltCallback configured, void* context);
static bool SolidSyslog_ReportingAlwaysOn(void* context);
static void SolidSyslog_InstallEventReporting(
    struct SolidSyslog* self,
    SolidSyslogEventReportingFunction configured,
    void* context
);
static enum SolidSyslogServiceStatus SolidSyslog_ProcessMessages(struct SolidSyslog* self);
static void SolidSyslog_ResetToDefaults(struct SolidSyslog* self);

void SolidSyslog_Initialise(
    struct SolidSyslog* self,
    const struct SolidSyslogConfig* config,
    SolidSyslogFormatterStorage* recordStorage,
    size_t recordCapacity
)
{
    SolidSyslog_ResetToDefaults(self);
    self->RecordStorage = recordStorage;
    self->Record = SolidSyslogFormatter_Bytes(SolidSyslogFormatter_Create(recordStorage, recordCapacity));
    self->RecordCapacity = recordCapacity;
    SolidSyslog_InstallBuffer(self, config->Buffer);
    SolidSyslog_InstallSender(self, config->Sender);
    SolidSyslog_InstallStore(self, config->Store);
    SolidSyslog_InstallClock(self, config->Clock);
    SolidSyslog_InstallHostname(self, config->GetHostname, config->GetHostnameContext);
    SolidSyslog_InstallAppName(self, config->GetAppName, config->GetAppNameContext);
    SolidSyslog_InstallProcessId(self, config->GetProcessId, config->GetProcessIdContext);
    SolidSyslog_InstallStructuredData(self, config->Sd, config->SdCount);
    SolidSyslog_InstallDiscardPolicy(self, config->GetDiscardPolicy, config->DiscardPolicyContext);
    SolidSyslog_InstallHalt(self, config->OnHalt, config->HaltContext);
    SolidSyslog_InstallScratch(self, config->Scratch);
    SolidSyslog_InstallEventReporting(self, config->IsEventReportingOn, config->EventReportingContext);
}

void SolidSyslog_Cleanup(struct SolidSyslog* self)
{
    /* Reset to safe defaults so a stale-handle Log/Service after Destroy is a
     * silent no-op rather than a NULL-fn-pointer crash. The slot is then
     * re-claimable by the next SolidSyslog_Create. */
    SolidSyslog_ResetToDefaults(self);
}

static void SolidSyslog_ResetToDefaults(struct SolidSyslog* self)
{
    self->Buffer = SolidSyslogNullBuffer_Get();
    self->Sender = SolidSyslogNullSender_Get();
    self->Store = SolidSyslogNullStore_Get();
    self->Scratch = SolidSyslogStackScratch_Get();
    self->Format.Clock = SolidSyslog_NullClock;
    self->Format.GetHostname = SolidSyslog_NullHeaderField;
    self->Format.GetHostnameContext = NULL;
    self->Format.GetAppName = SolidSyslog_NullHeaderField;
    self->Format.GetAppNameContext = NULL;
    self->Format.GetProcessId = SolidSyslog_NullHeaderField;
    self->Format.GetProcessIdContext = NULL;
    self->Format.Sd = NULL;
    self->Format.SdCount = 0;
    self->RecordStorage = NULL;
    self->Record = NULL;
    self->RecordCapacity = 0U;
    self->RecordLength = 0U;
    self->HoldsRecord = false;
    self->HeldRecordIsReport = false;
    self->HeldRecordLossReported = false;
    self->GetDiscardPolicy = SolidSyslogDiscardPolicy_Default;
    self->DiscardPolicyContext = NULL;
    self->OnHalt = SolidSyslog_NullHalt;
    self->HaltContext = NULL;
    self->IsEventReportingOn = SolidSyslog_ReportingAlwaysOn;
    self->EventReportingContext = NULL;
    self->ReportTurnedDown = false;
    self->Halted = false;
}

/* No-op defaults for the header-field callbacks. No public Null equivalent
 * exists for the function-pointer typedefs. */
static void SolidSyslog_NullClock(struct SolidSyslogTimestamp* ts)
{
    (void) ts;
}

static void SolidSyslog_NullHeaderField(struct SolidSyslogHeaderField* field, void* context)
{
    (void) field;
    (void) context;
}

static void SolidSyslog_NullHalt(void* context)
{
    (void) context;
}

static bool SolidSyslog_ReportingAlwaysOn(void* context)
{
    (void) context;
    return true;
}

static void SolidSyslog_InstallBuffer(struct SolidSyslog* self, struct SolidSyslogBuffer* configured)
{
    if (configured == NULL)
    {
        SolidSyslog_Report(
            SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_ERROR_CREATE_NULL_BUFFER
        );
    }
    else
    {
        self->Buffer = configured;
    }
}

static void SolidSyslog_InstallSender(struct SolidSyslog* self, struct SolidSyslogSender* configured)
{
    if (configured == NULL)
    {
        SolidSyslog_Report(
            SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_ERROR_CREATE_NULL_SENDER
        );
    }
    else
    {
        self->Sender = configured;
    }
}

static void SolidSyslog_InstallStore(struct SolidSyslog* self, struct SolidSyslogStore* configured)
{
    if (configured == NULL)
    {
        SolidSyslog_Report(
            SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_ERROR_CREATE_NULL_STORE
        );
    }
    else
    {
        self->Store = configured;
    }
}

static void SolidSyslog_InstallClock(struct SolidSyslog* self, SolidSyslogClockFunction configured)
{
    if (configured != NULL)
    {
        self->Format.Clock = configured;
    }
}

static void SolidSyslog_InstallHostname(
    struct SolidSyslog* self,
    SolidSyslogHeaderFieldFunction configured,
    void* context
)
{
    if (configured != NULL)
    {
        self->Format.GetHostname = configured;
        self->Format.GetHostnameContext = context;
    }
}

static void SolidSyslog_InstallAppName(
    struct SolidSyslog* self,
    SolidSyslogHeaderFieldFunction configured,
    void* context
)
{
    if (configured != NULL)
    {
        self->Format.GetAppName = configured;
        self->Format.GetAppNameContext = context;
    }
}

static void SolidSyslog_InstallProcessId(
    struct SolidSyslog* self,
    SolidSyslogHeaderFieldFunction configured,
    void* context
)
{
    if (configured != NULL)
    {
        self->Format.GetProcessId = configured;
        self->Format.GetProcessIdContext = context;
    }
}

static void SolidSyslog_InstallDiscardPolicy(
    struct SolidSyslog* self,
    SolidSyslogDiscardPolicyFunction configured,
    void* context
)
{
    if (configured != NULL)
    {
        self->GetDiscardPolicy = configured;
        self->DiscardPolicyContext = context;
    }
}

static void SolidSyslog_InstallHalt(struct SolidSyslog* self, SolidSyslogHaltCallback configured, void* context)
{
    if (configured != NULL)
    {
        self->OnHalt = configured;
        self->HaltContext = context;
    }
}

/* NULL means the stack scratch, which is what every logger had before the
 * field existed; nothing to report. */
static void SolidSyslog_InstallScratch(struct SolidSyslog* self, struct SolidSyslogScratch* configured)
{
    if (configured != NULL)
    {
        self->Scratch = configured;
    }
}

static void SolidSyslog_InstallEventReporting(
    struct SolidSyslog* self,
    SolidSyslogEventReportingFunction configured,
    void* context
)
{
    if (configured != NULL)
    {
        self->IsEventReportingOn = configured;
        self->EventReportingContext = context;
    }
}

static void SolidSyslog_InstallStructuredData(
    struct SolidSyslog* self,
    struct SolidSyslogStructuredData** configured,
    size_t count
)
{
    if ((configured == NULL) && (count > 0U))
    {
        /* Inconsistent pairing - the formatter would dereference Sd[i] for
         * i < SdCount against a NULL array. Report and leave the reset
         * defaults (no SD) in place so Log() degrades safely. */
        SolidSyslog_Report(
            SOLIDSYSLOG_BAD_CONFIG_FATAL_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_CONFIG,
            SOLIDSYSLOG_ERROR_CREATE_INCONSISTENT_SD
        );
    }
    else
    {
        self->Format.Sd = configured;
        self->Format.SdCount = count;
    }
}

enum SolidSyslogServiceStatus SolidSyslog_Service(struct SolidSyslog* handle)
{
    enum SolidSyslogServiceStatus status = SOLIDSYSLOG_SERVICE_IDLE;

    if (handle == NULL)
    {
        SolidSyslog_Report(
            SOLIDSYSLOG_BAD_ARGUMENT_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_ARGUMENT,
            SOLIDSYSLOG_ERROR_SERVICE_NULL_HANDLE
        );
    }
    else
    {
        SolidSyslog_ResumeIfPolicyChanged(handle);

        if (SolidSyslog_IsServiceEnabled(handle))
        {
            status = SolidSyslog_ProcessMessages(handle);
        }
        else
        {
            /* Halted - nothing moves until the policy answers otherwise. */
            status = SOLIDSYSLOG_SERVICE_HALTED;
        }
    }

    return status;
}

/* A halt stands while the policy still answers Halt; a device whose policy
 * changes resumes from where it stopped, with everything it held still there. */
static inline void SolidSyslog_ResumeIfPolicyChanged(struct SolidSyslog* self)
{
    self->Halted = self->Halted && SolidSyslog_PolicyHalts(self);
}

static inline bool SolidSyslog_IsServiceEnabled(const struct SolidSyslog* self)
{
    return !self->Halted;
}

static inline bool SolidSyslog_PolicyHalts(struct SolidSyslog* self)
{
    return self->GetDiscardPolicy(self->DiscardPolicyContext) == SOLIDSYSLOG_DISCARD_POLICY_HALT;
}

/* A pass that halts says so at once, and sends nothing after the refusal
 * that halted it. */
static enum SolidSyslogServiceStatus SolidSyslog_ProcessMessages(struct SolidSyslog* self)
{
    self->ReportTurnedDown = false;
    bool drained = SolidSyslog_DrainBufferIntoStore(self);
    bool storeBlocked = !self->Halted && SolidSyslog_SendOneFromStore(self);
    enum SolidSyslogServiceStatus status = SOLIDSYSLOG_SERVICE_IDLE;

    if (self->Halted)
    {
        status = SOLIDSYSLOG_SERVICE_HALTED;
    }
    else if (SolidSyslog_IsBlocked(self, storeBlocked, drained))
    {
        status = SOLIDSYSLOG_SERVICE_BLOCKED;
    }
    else if (drained || SolidSyslogStore_HasUnsent(self->Store) || SolidSyslog_IsReportWaiting(self))
    {
        status = SOLIDSYSLOG_SERVICE_READY;
    }
    else
    {
        /* Buffer idle, nothing stored, no failed send - IDLE (initial value). */
    }

    return status;
}

/* Eagerly drain the buffer so the producer-side shock absorber stays small while
 * the sender is slow or down - a store that retains records then takes the
 * backlog, and the buffer applies the discard policy only to what it cannot
 * hand on. A pass deals with the record held from the last one before it reads
 * the buffer, and stops at a record it cannot deal with, so nothing overtakes a
 * record waiting on the sender, or at the refusal that halts the logger. Says
 * whether anything was taken from the buffer. */
static inline bool SolidSyslog_DrainBufferIntoStore(struct SolidSyslog* self)
{
    bool drained = false;
    SolidSyslog_ApplyPolicyToHeldRecordIfBufferFull(self);
    bool dealtWith = !self->Halted && SolidSyslog_DealWithHeldRecord(self);

    if (dealtWith && !self->Halted)
    {
        dealtWith = SolidSyslog_ReportPendingEvents(self);
    }

    while (dealtWith && !self->Halted && SolidSyslog_HoldNextRecord(self))
    {
        drained = true;
        dealtWith = SolidSyslog_DealWithHeldRecord(self);
    }

    return drained;
}

/* A record held from the last pass is the head of the whole queue, with the
 * buffer behind it. A buffer that has filled meanwhile has lost records under
 * its policy already, and the same policy decides the head: under Oldest it is
 * the next to go, so the buffer's head is taken in its place; under Halt the
 * logger stops, as it does when the store refuses a record; under Newest the
 * held record is kept and retried. A report is kept under Oldest too: it is
 * the account of a loss rather than a record the buffer took, and dropping it
 * would lose the account and start a loss of its own. */
static inline void SolidSyslog_ApplyPolicyToHeldRecordIfBufferFull(struct SolidSyslog* self)
{
    if (self->HoldsRecord && SolidSyslogBuffer_IsFull(self->Buffer))
    {
        enum SolidSyslogDiscardPolicy policy = self->GetDiscardPolicy(self->DiscardPolicyContext);

        if ((policy == SOLIDSYSLOG_DISCARD_POLICY_OLDEST) && !self->HeldRecordIsReport)
        {
            SolidSyslog_DropHeldRecord(self);
        }
        else if (policy == SOLIDSYSLOG_DISCARD_POLICY_HALT)
        {
            SolidSyslog_Halt(self);
        }
        else
        {
            /* Newest, or a report under Oldest: the held record keeps its place. */
        }
    }
}

/* The run is marked before the report, so a handler that logs cannot repeat
 * it; it ends when a held record from the buffer is dealt with. */
static inline void SolidSyslog_DropHeldRecord(struct SolidSyslog* self)
{
    bool runStarted = !self->HeldRecordLossReported;
    self->HoldsRecord = false;
    self->HeldRecordLossReported = true;

    if (runStarted)
    {
        SolidSyslog_Report(
            SOLIDSYSLOG_SEVERITY_WARNING,
            SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST,
            SOLIDSYSLOG_ERROR_HELD_RECORD_DROPPED
        );
    }
}

/* Takes the next record from the buffer into the slot. Says whether one was
 * there to take. */
static bool SolidSyslog_HoldNextRecord(struct SolidSyslog* self)
{
    self->HoldsRecord = SolidSyslogBuffer_Read(self->Buffer, self->Record, self->RecordCapacity, &self->RecordLength);
    self->HeldRecordIsReport = false;

    return self->HoldsRecord;
}

/* Deals with the held record, if there is one. Says whether the slot is free
 * again; a record the sender refused stays held for the next pass. */
static bool SolidSyslog_DealWithHeldRecord(struct SolidSyslog* self)
{
    if (self->HoldsRecord)
    {
        self->HoldsRecord = !SolidSyslog_StoreOrSenderTakesHeldRecord(self);
        self->HeldRecordLossReported = self->HeldRecordLossReported && !SolidSyslog_IsBufferRecordDealtWith(self);
    }

    return !self->HoldsRecord;
}

/* Only a record from the buffer ends a run of drops. The report of a drop is
 * made in the pass that dropped it, so a report that ended the run would have
 * every drop in it reported again. */
static inline bool SolidSyslog_IsBufferRecordDealtWith(const struct SolidSyslog* self)
{
    return !self->HoldsRecord && !self->HeldRecordIsReport;
}

/* Offers the held record to the store, and to the sender only when the store
 * has not dealt with it - unless the store's refusal halted the logger, which
 * sends nothing after it. */
static inline bool SolidSyslog_StoreOrSenderTakesHeldRecord(struct SolidSyslog* self)
{
    return SolidSyslog_StoreDealsWithHeldRecord(self) ||
           (!self->Halted && SolidSyslogSender_Send(self->Sender, self->Record, self->RecordLength));
}

/* Offers the held record to the store. A store that retains records has dealt
 * with a record from the buffer either way: it kept it, it lost it and reported
 * why, or its discard policy refused it and reported the loss - letting a
 * refused record escape to the sender would put it ahead of older stored ones
 * once the sender recovered. A report is the account of a loss, and losing it
 * would lose the account, so the store has dealt with one only by keeping it:
 * one it lost or refused goes to the sender, ahead of the stored records, and
 * is held while the sender refuses it, or while the refusal has the logger
 * halted. A store that keeps nothing never had either, so it is still to be
 * sent. */
static inline bool SolidSyslog_StoreDealsWithHeldRecord(struct SolidSyslog* self)
{
    enum SolidSyslogStoreWriteResult result = SolidSyslogStore_Write(self->Store, self->Record, self->RecordLength);
    bool refused = (result == SOLIDSYSLOG_STORE_WRITE_NOT_RETAINED) && !SolidSyslogStore_IsTransient(self->Store);
    bool notKept = refused || (result == SOLIDSYSLOG_STORE_WRITE_LOST);

    if (refused)
    {
        SolidSyslog_HaltIfPolicyHalts(self);
    }

    return (result == SOLIDSYSLOG_STORE_WRITE_KEPT) || (notKept && !self->HeldRecordIsReport);
}

/* A refusal is the discard policy at work; under Halt it is also where the
 * logger stops. The record refused is gone either way, so the halt is asked
 * about after the store has reported it, and the callback hears of it after
 * that. */
static inline void SolidSyslog_HaltIfPolicyHalts(struct SolidSyslog* self)
{
    if (SolidSyslog_PolicyHalts(self))
    {
        SolidSyslog_Halt(self);
    }
}

static inline void SolidSyslog_Halt(struct SolidSyslog* self)
{
    self->Halted = true;
    SolidSyslog_Report(
        SOLIDSYSLOG_SEVERITY_ERROR,
        SOLIDSYSLOG_CAT_LOGGER_HALTED,
        SOLIDSYSLOG_ERROR_HALTED_STORE_REFUSED
    );
    self->OnHalt(self->HaltContext);
}

/* Walks the table once, in its order, so a cause is reported ahead of what it
 * caused. Each report is made in the free slot and dealt with before the next
 * is made, and a kind raised meanwhile at a slot already walked waits for the
 * next pass. The walk ends at a report the store and sender did not take, at a
 * halt, or at the switch's first off, leaving every kind not yet reported
 * waiting. Says whether the slot is free. */
static inline bool SolidSyslog_ReportPendingEvents(struct SolidSyslog* self)
{
    bool walking = SolidSyslog_HasRecordSlot(self);

    for (size_t index = 0U; (index < SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT) && walking; index++)
    {
        walking = SolidSyslog_ReportIfPending(self, index);
    }

    return !self->HoldsRecord;
}

/* The fallback SolidSyslog_Create returns carries no slot, so it takes no
 * event and leaves each one for a logger that can report it. */
static inline bool SolidSyslog_HasRecordSlot(const struct SolidSyslog* self)
{
    return self->RecordStorage != NULL;
}

/* Only a kind with an event waiting asks the integrator's switch, and an event
 * it turns down stays waiting for a pass when the switch is on. Says whether
 * the walk goes on. */
static inline bool SolidSyslog_ReportIfPending(struct SolidSyslog* self, size_t index)
{
    bool goesOn = true;

    if (SolidSyslogPendingEvents_IsPending(index))
    {
        goesOn = self->IsEventReportingOn(self->EventReportingContext);
        self->ReportTurnedDown = !goesOn;

        if (goesOn)
        {
            SolidSyslog_HoldPendingReport(self, index);
            goesOn = SolidSyslog_DealWithHeldRecord(self) && !self->Halted;
        }
    }

    return goesOn;
}

/* Takes the kind's event into the free slot as a report, formatted as a record
 * of the logger's own. The switch is integrator code, asked after the kind was
 * seen pending: should another logger's pass take the event meanwhile, this
 * take finds nothing and the slot stays free, and what was formatted into it
 * is never sent. */
static inline void SolidSyslog_HoldPendingReport(struct SolidSyslog* self, size_t index)
{
    struct SolidSyslogErrorEvent event;
    self->HoldsRecord = SolidSyslogPendingEvents_Take(index, &event);
    self->HeldRecordIsReport = self->HoldsRecord;
    SolidSyslog_FormatReport(self, &event);
}

static inline void SolidSyslog_FormatReport(struct SolidSyslog* self, const struct SolidSyslogErrorEvent* event)
{
    /* The MSG of every report; its MSGID and the library's SD element say
     * which event it was. */
    static const char reportMsg[] = "SolidSyslog event";
    struct SolidSyslogMessage message =
        {SOLIDSYSLOG_FACILITY_SYSLOG, event->Severity, SolidSyslogErrorCategory_Name(event->Category), reportMsg};
    struct SolidSyslogEventSd eventSd;
    struct SolidSyslogStructuredData* sd = SolidSyslogEventSd_Initialise(&eventSd, event);
    struct SolidSyslogFormatter* formatter = SolidSyslogFormatter_Create(self->RecordStorage, self->RecordCapacity);

    SolidSyslogMessageFormatter_Format(formatter, &message, &self->Format, &sd, 1U);
    /* Trims a truncated multi-byte tail in place; Record already points here. */
    (void) SolidSyslogFormatter_AsFormattedBuffer(formatter);
    self->RecordLength = SolidSyslogFormatter_Length(formatter);
}

/* Sends one record the store hands out. The slot is the staging buffer; the
 * record stays the store's, re-read next pass if the sender refuses it. */
static inline bool SolidSyslog_SendOneFromStore(struct SolidSyslog* self)
{
    bool blocked = false;

    if (SolidSyslog_TakeNextFromStore(self))
    {
        if (SolidSyslogSender_Send(self->Sender, self->Record, self->RecordLength))
        {
            SolidSyslogStore_MarkSent(self->Store);
        }
        else
        {
            blocked = true;
        }
    }
    else
    {
        /* Records the store holds but cannot hand out yet. */
        blocked = SolidSyslogStore_HasUnsent(self->Store);
    }

    return blocked;
}

/* Reads the store's oldest unsent record into the slot. Nothing is asked of the
 * store while Service holds a record of its own: the slot is taken, and a store
 * that keeps nothing has nothing to hand out anyway. */
static inline bool SolidSyslog_TakeNextFromStore(struct SolidSyslog* self)
{
    return !self->HoldsRecord &&
           SolidSyslogStore_ReadNextUnsent(self->Store, self->Record, self->RecordCapacity, &self->RecordLength);
}

/* A record held for a failed direct send blocks the pass outright: the buffer
 * waits behind it. A store that could not move a record blocks it only when the
 * buffer stayed idle - buffer activity out-ranks a blocked store, so a down
 * sender never demotes the loop out of "keep draining" while there is a store to
 * drain into. */
static inline bool SolidSyslog_IsBlocked(struct SolidSyslog* self, bool storeBlocked, bool drained)
{
    return self->HoldsRecord || (storeBlocked && !drained);
}

/* An event raised after the walk passed its slot - by the reports themselves,
 * the buffer or the store - waits for the next pass, so it keeps the loop
 * going. One the switch turned down this pass does not: the switch is asked
 * again only on a pass, so a loop told to keep going would spin until it
 * answered otherwise. The fallback has no slot to report in. */
static inline bool SolidSyslog_IsReportWaiting(const struct SolidSyslog* self)
{
    return SolidSyslog_HasRecordSlot(self) && !self->ReportTurnedDown && SolidSyslogPendingEvents_AnyPending();
}

void SolidSyslog_Log(struct SolidSyslog* handle, const struct SolidSyslogMessage* message)
{
    SolidSyslog_DoLog(handle, message, NULL, 0);
}

void SolidSyslog_LogWithSd(
    struct SolidSyslog* handle,
    const struct SolidSyslogMessage* message,
    struct SolidSyslogStructuredData** sd,
    size_t sdCount
)
{
    SolidSyslog_DoLog(handle, message, sd, sdCount);
}

static void SolidSyslog_DoLog(
    struct SolidSyslog* handle,
    const struct SolidSyslogMessage* message,
    struct SolidSyslogStructuredData** sd,
    size_t sdCount
)
{
    if (handle == NULL)
    {
        SolidSyslog_Report(
            SOLIDSYSLOG_BAD_ARGUMENT_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_ARGUMENT,
            SOLIDSYSLOG_ERROR_LOG_NULL_HANDLE
        );
    }
    else if (message == NULL)
    {
        SolidSyslog_Report(
            SOLIDSYSLOG_BAD_ARGUMENT_SEVERITY,
            SOLIDSYSLOG_CAT_BAD_ARGUMENT,
            SOLIDSYSLOG_ERROR_LOG_NULL_MESSAGE
        );
    }
    else
    {
        /* Inconsistent pairing - the formatter would dereference sd[i] for
           i < sdCount against a NULL array. Report and drop to no per-message
           SD so the message still logs (degrade safely), mirroring the
           Create-time guard in SolidSyslog_InstallStructuredData. */
        size_t safeSdCount = sdCount;
        if ((sd == NULL) && (sdCount > 0U))
        {
            SolidSyslog_Report(
                SOLIDSYSLOG_BAD_ARGUMENT_SEVERITY,
                SOLIDSYSLOG_CAT_BAD_ARGUMENT,
                SOLIDSYSLOG_ERROR_LOG_INCONSISTENT_SD
            );
            safeSdCount = 0U;
        }

        struct LogRequest request = {handle, message, sd, safeSdCount};
        handle->Scratch->WithScratch(handle->Scratch, SolidSyslog_FormatIntoScratch, &request);
    }
}

static void SolidSyslog_FormatIntoScratch(SolidSyslogFormatterStorage* storage, size_t messageBytes, void* context)
{
    struct LogRequest* request = (struct LogRequest*) context;
    struct SolidSyslogFormatter* f = SolidSyslogFormatter_Create(storage, messageBytes);

    SolidSyslogMessageFormatter_Format(f, request->Message, &request->Handle->Format, request->Sd, request->SdCount);
    SolidSyslogBuffer_Write(
        request->Handle->Buffer,
        SolidSyslogFormatter_AsFormattedBuffer(f),
        SolidSyslogFormatter_Length(f)
    );
}

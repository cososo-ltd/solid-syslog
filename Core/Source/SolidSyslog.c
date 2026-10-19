/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslog.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogDiscardPolicy.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrorCategory.h"
#include "SolidSyslogErrors.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogMessageFormatter.h"
#include "SolidSyslogNullBuffer.h"
#include "SolidSyslogNullSender.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogPrivate.h"
#include "SolidSyslogHeaderFieldFunction.h"
#include "SolidSyslogSender.h"
#include "SolidSyslogStore.h"
#include "SolidSyslogTimestamp.h"
#include "SolidSyslogTunables.h"

const struct SolidSyslogErrorSource SolidSyslogErrorSource = {"SolidSyslog"};

struct SolidSyslogBuffer;
struct SolidSyslogHeaderField;
struct SolidSyslogSender;
struct SolidSyslogStore;
struct SolidSyslogStructuredData;

static inline bool SolidSyslog_DrainBufferIntoStore(struct SolidSyslog* self);
static bool SolidSyslog_HoldNextRecord(struct SolidSyslog* self);
static bool SolidSyslog_DealWithHeldRecord(struct SolidSyslog* self);
static inline bool SolidSyslog_StoreOrSenderTakesHeldRecord(struct SolidSyslog* self);
static inline bool SolidSyslog_StoreDealsWithHeldRecord(struct SolidSyslog* self);
static inline void SolidSyslog_HaltIfPolicyHalts(struct SolidSyslog* self);
static inline bool SolidSyslog_SendOneFromStore(struct SolidSyslog* self);
static inline bool SolidSyslog_TakeNextFromStore(struct SolidSyslog* self);
static inline bool SolidSyslog_IsBlocked(struct SolidSyslog* self, bool storeBlocked, bool drained);
static void SolidSyslog_DoLog(
    struct SolidSyslog* handle,
    const struct SolidSyslogMessage* message,
    struct SolidSyslogStructuredData** sd,
    size_t sdCount
);
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
static inline bool SolidSyslog_IsServiceEnabled(struct SolidSyslog* self);
static inline bool SolidSyslog_PolicyHalts(struct SolidSyslog* self);
static void SolidSyslog_NullClock(struct SolidSyslogTimestamp* ts);
static void SolidSyslog_NullHeaderField(struct SolidSyslogHeaderField* field, void* context);
static void SolidSyslog_NullHalt(void* context);
static void SolidSyslog_InstallHalt(struct SolidSyslog* self, SolidSyslogHaltCallback configured, void* context);
static enum SolidSyslogServiceStatus SolidSyslog_ProcessMessages(struct SolidSyslog* self);
static void SolidSyslog_ResetToDefaults(struct SolidSyslog* self);

void SolidSyslog_Initialise(struct SolidSyslog* self, const struct SolidSyslogConfig* config)
{
    SolidSyslog_ResetToDefaults(self);
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
    self->Format.Clock = SolidSyslog_NullClock;
    self->Format.GetHostname = SolidSyslog_NullHeaderField;
    self->Format.GetHostnameContext = NULL;
    self->Format.GetAppName = SolidSyslog_NullHeaderField;
    self->Format.GetAppNameContext = NULL;
    self->Format.GetProcessId = SolidSyslog_NullHeaderField;
    self->Format.GetProcessIdContext = NULL;
    self->Format.Sd = NULL;
    self->Format.SdCount = 0;
    self->RecordLength = 0U;
    self->HoldsRecord = false;
    self->GetDiscardPolicy = SolidSyslogDiscardPolicy_Default;
    self->DiscardPolicyContext = NULL;
    self->OnHalt = SolidSyslog_NullHalt;
    self->HaltContext = NULL;
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
    else if (SolidSyslog_IsServiceEnabled(handle))
    {
        status = SolidSyslog_ProcessMessages(handle);
    }
    else
    {
        /* Halted - nothing moves until the policy answers otherwise. */
        status = SOLIDSYSLOG_SERVICE_HALTED;
    }

    return status;
}

/* A halt stands while the policy still answers Halt; a device whose policy
 * changes resumes from where it stopped, with everything it held still there. */
static inline bool SolidSyslog_IsServiceEnabled(struct SolidSyslog* self)
{
    self->Halted = self->Halted && SolidSyslog_PolicyHalts(self);

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
    else if (drained || SolidSyslogStore_HasUnsent(self->Store))
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
 * the sender is slow or down - overflow then engages the store's discard policy
 * rather than silently dropping at the buffer. A pass deals with the record held
 * from the last one before it reads the buffer, and stops at a record it cannot
 * deal with, so nothing overtakes a record waiting on the sender, or at the
 * refusal that halts the logger. Says whether anything was taken from the
 * buffer. */
static inline bool SolidSyslog_DrainBufferIntoStore(struct SolidSyslog* self)
{
    bool drained = false;
    bool dealtWith = SolidSyslog_DealWithHeldRecord(self);

    while (dealtWith && !self->Halted && SolidSyslog_HoldNextRecord(self))
    {
        drained = true;
        dealtWith = SolidSyslog_DealWithHeldRecord(self);
    }

    return drained;
}

/* Takes the next record from the buffer into the slot. Says whether one was
 * there to take. */
static bool SolidSyslog_HoldNextRecord(struct SolidSyslog* self)
{
    self->HoldsRecord = SolidSyslogBuffer_Read(self->Buffer, self->Record, sizeof(self->Record), &self->RecordLength);

    return self->HoldsRecord;
}

/* Deals with the held record, if there is one. Says whether the slot is free
 * again; a record the sender refused stays held for the next pass. */
static bool SolidSyslog_DealWithHeldRecord(struct SolidSyslog* self)
{
    if (self->HoldsRecord)
    {
        self->HoldsRecord = !SolidSyslog_StoreOrSenderTakesHeldRecord(self);
    }

    return !self->HoldsRecord;
}

/* Offers the held record to the store, and to the sender only when the store
 * has not dealt with it. */
static inline bool SolidSyslog_StoreOrSenderTakesHeldRecord(struct SolidSyslog* self)
{
    return SolidSyslog_StoreDealsWithHeldRecord(self) ||
           SolidSyslogSender_Send(self->Sender, self->Record, self->RecordLength);
}

/* Offers the held record to the store. A store that retains records has dealt
 * with it either way: it kept it, or its discard policy refused it and reported
 * the loss - letting a refused record escape to the sender would put it ahead
 * of older stored ones once the sender recovered. A store that keeps nothing
 * never had it, so the record is still to be sent. */
static inline bool SolidSyslog_StoreDealsWithHeldRecord(struct SolidSyslog* self)
{
    bool kept = SolidSyslogStore_Write(self->Store, self->Record, self->RecordLength);
    bool refused = !kept && !SolidSyslogStore_IsTransient(self->Store);

    if (refused)
    {
        SolidSyslog_HaltIfPolicyHalts(self);
    }

    return kept || refused;
}

/* A refusal is the discard policy at work; under Halt it is also where the
 * logger stops. The record refused is gone either way, so the halt is asked
 * about after the store has reported it, and the callback hears of it after
 * that. */
static inline void SolidSyslog_HaltIfPolicyHalts(struct SolidSyslog* self)
{
    self->Halted = SolidSyslog_PolicyHalts(self);

    if (self->Halted)
    {
        self->OnHalt(self->HaltContext);
    }
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
           SolidSyslogStore_ReadNextUnsent(self->Store, self->Record, sizeof(self->Record), &self->RecordLength);
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

        SolidSyslogFormatterStorage storage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(SOLIDSYSLOG_MAX_MESSAGE_SIZE)];
        struct SolidSyslogFormatter* f = SolidSyslogFormatter_Create(storage, SOLIDSYSLOG_MAX_MESSAGE_SIZE);

        SolidSyslogMessageFormatter_Format(f, message, &handle->Format, sd, safeSdCount);
        SolidSyslogBuffer_Write(
            handle->Buffer,
            SolidSyslogFormatter_AsFormattedBuffer(f),
            SolidSyslogFormatter_Length(f)
        );
    }
}

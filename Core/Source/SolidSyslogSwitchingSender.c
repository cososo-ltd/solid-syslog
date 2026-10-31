/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogNullSender.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSender.h"
#include "SolidSyslogSenderCategories.h"
#include "SolidSyslogSenderDefinition.h"
#include "SolidSyslogSwitchingSender.h"
#include "SolidSyslogSwitchingSenderErrors.h"
#include "SolidSyslogSwitchingSenderPrivate.h"

const struct SolidSyslogErrorSource SolidSyslogSwitchingSenderErrorSource = {
    "SwitchingSender",
    SolidSyslogSwitchingSenderErrors_Name
};

static bool SwitchingSender_Send(struct SolidSyslogSender* base, const void* buffer, size_t size);
static void SwitchingSender_Disconnect(struct SolidSyslogSender* base);

static inline struct SolidSyslogSwitchingSender* SwitchingSender_SelfFromBase(struct SolidSyslogSender* base);
static inline void SwitchingSender_SelectSender(struct SolidSyslogSwitchingSender* self);
static inline bool SwitchingSender_SenderChanged(
    const struct SolidSyslogSwitchingSender* self,
    const struct SolidSyslogSender* requestedSender
);
static inline void SwitchingSender_SwitchTo(
    struct SolidSyslogSwitchingSender* self,
    struct SolidSyslogSender* newCurrent
);
static inline struct SolidSyslogSender* SwitchingSender_RequestedSender(struct SolidSyslogSwitchingSender* self);
static inline void SwitchingSender_ReportOutOfRange(struct SolidSyslogSwitchingSender* self);

void SolidSyslogSwitchingSender_Initialise(
    struct SolidSyslogSender* base,
    const struct SolidSyslogSwitchingSenderConfig* config
)
{
    struct SolidSyslogSwitchingSender* self = SwitchingSender_SelfFromBase(base);
    self->Base.Send = SwitchingSender_Send;
    self->Base.Disconnect = SwitchingSender_Disconnect;
    self->Config = *config;
    self->CurrentSender = SolidSyslogNullSender_Get();
    self->OutOfRangeReported = false;
}

void SolidSyslogSwitchingSender_Cleanup(struct SolidSyslogSender* base)
{
    /* Overwrite the abstract base with the shared NullSender vtable so use-after-destroy
     * is a safe no-op. SwitchingSender does not own its inner senders' connections, so
     * unlike UdpSender/StreamSender there is nothing to disconnect first. Derived fields
     * are private to this TU; the next SolidSyslogSwitchingSender_Initialise overwrites them. */
    *base = *SolidSyslogNullSender_Get();
}

static bool SwitchingSender_Send(struct SolidSyslogSender* base, const void* buffer, size_t size)
{
    struct SolidSyslogSwitchingSender* self = SwitchingSender_SelfFromBase(base);
    SwitchingSender_SelectSender(self);
    return SolidSyslogSender_Send(self->CurrentSender, buffer, size);
}

static void SwitchingSender_Disconnect(struct SolidSyslogSender* base)
{
    struct SolidSyslogSwitchingSender* self = SwitchingSender_SelfFromBase(base);
    SolidSyslogSender_Disconnect(self->CurrentSender);
}

static inline struct SolidSyslogSwitchingSender* SwitchingSender_SelfFromBase(struct SolidSyslogSender* base)
{
    return (struct SolidSyslogSwitchingSender*) base;
}

static inline void SwitchingSender_SelectSender(struct SolidSyslogSwitchingSender* self)
{
    struct SolidSyslogSender* requestedSender = SwitchingSender_RequestedSender(self);

    if (SwitchingSender_SenderChanged(self, requestedSender))
    {
        SwitchingSender_SwitchTo(self, requestedSender);
    }
}

static inline bool SwitchingSender_SenderChanged(
    const struct SolidSyslogSwitchingSender* self,
    const struct SolidSyslogSender* requestedSender
)
{
    return requestedSender != self->CurrentSender;
}

static inline void SwitchingSender_SwitchTo(
    struct SolidSyslogSwitchingSender* self,
    struct SolidSyslogSender* newCurrent
)
{
    SolidSyslogSender_Disconnect(self->CurrentSender);
    self->CurrentSender = newCurrent;
}

/* Out-of-range selector index (including empty-array case) resolves to the
 * shared NullSender. NullSender.Send returns true so the Service algorithm
 * drops the message - a misconfigured selector must not retain messages in
 * the Store - and the loss is reported. */
static inline struct SolidSyslogSender* SwitchingSender_RequestedSender(struct SolidSyslogSwitchingSender* self)
{
    struct SolidSyslogSender* result = SolidSyslogNullSender_Get();
    uint8_t index = self->Config.Selector(self->Config.SelectorContext);

    if (index < self->Config.SenderCount)
    {
        result = self->Config.Senders[index];
        self->OutOfRangeReported = false;
    }
    else
    {
        SwitchingSender_ReportOutOfRange(self);
    }

    return result;
}

/* One report stands for a run of out-of-range indexes; the next index in range
 * ends the run. Checked where the index is read rather than on a switch, as
 * CurrentSender starts as the NullSender and a selector out of range from the
 * first Send never switches. */
static inline void SwitchingSender_ReportOutOfRange(struct SolidSyslogSwitchingSender* self)
{
    if (!self->OutOfRangeReported)
    {
        SwitchingSender_Report(
            SOLIDSYSLOG_SEVERITY_CRITICAL,
            SOLIDSYSLOG_CAT_SENDER_RECORDS_LOST,
            SOLIDSYSLOG_SWITCHING_SENDER_ERROR_SELECTOR_OUT_OF_RANGE
        );
        self->OutOfRangeReported = true;
    }
}

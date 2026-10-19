/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGPRIVATE_H
#define SOLIDSYSLOGPRIVATE_H

#include <stddef.h>

#include "SolidSyslogDiscardPolicy.h"
#include "SolidSyslogError.h"
#include "SolidSyslogErrors.h"
#include "SolidSyslogMessageFormatter.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTimestamp.h"
#include "SolidSyslogTunables.h"

struct SolidSyslogBuffer;
struct SolidSyslogConfig;
struct SolidSyslogHeaderField;
struct SolidSyslogSender;
struct SolidSyslogStore;

struct SolidSyslog
{
    struct SolidSyslogBuffer* Buffer;
    struct SolidSyslogSender* Sender;
    struct SolidSyslogStore* Store;
    struct SolidSyslogMessageFormatterContext Format;
    /* The one record Service works on at a time: taken from the buffer, or read
     * from the store to send. It lives in the instance so that a record no store
     * will take can be kept across Service passes; HoldsRecord is true while one
     * is kept that way, and Service reads neither the buffer nor the store until
     * it has gone. Service alone uses the slot, from the one task that calls it;
     * Create and Destroy only empty it. */
    char Record[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    size_t RecordLength;
    bool HoldsRecord;
    SolidSyslogDiscardPolicyFunction GetDiscardPolicy;
    void* DiscardPolicyContext;
    /* Set when a retaining store refuses a record while the policy answers
     * Halt; Service then moves nothing until the answer changes. */
    bool Halted;
};

void SolidSyslog_Initialise(struct SolidSyslog* self, const struct SolidSyslogConfig* config);
void SolidSyslog_Cleanup(struct SolidSyslog* self);

static inline void SolidSyslog_Report(enum SolidSyslogSeverity severity, uint16_t category, enum SolidSyslogErrors code)
{
    SolidSyslog_Error(severity, &SolidSyslogErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGPRIVATE_H */

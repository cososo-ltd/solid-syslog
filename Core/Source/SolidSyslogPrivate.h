/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGPRIVATE_H
#define SOLIDSYSLOGPRIVATE_H

#include <stddef.h>

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
    /* The record Service has taken from the buffer and not yet dealt with. It
     * lives in the instance so that a record can be kept across Service passes.
     * Service alone uses it, from the one task that calls it; Create and
     * Destroy only empty it. HoldsRecord is true while a record is in the
     * slot: taken from the buffer this pass, or refused by the sender in an
     * earlier one and waiting to be sent. */
    char HeldRecord[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    size_t HeldRecordLength;
    bool HoldsRecord;
};

void SolidSyslog_Initialise(struct SolidSyslog* self, const struct SolidSyslogConfig* config);
void SolidSyslog_Cleanup(struct SolidSyslog* self);

static inline void SolidSyslog_Report(enum SolidSyslogSeverity severity, uint16_t category, enum SolidSyslogErrors code)
{
    SolidSyslog_Error(severity, &SolidSyslogErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGPRIVATE_H */

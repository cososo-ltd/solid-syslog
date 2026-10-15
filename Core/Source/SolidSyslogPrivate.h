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
     * lives here rather than on the Service task's stack, and only Service
     * touches it, from the one task that calls it. */
    char HeldRecord[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    size_t HeldRecordLength;
};

void SolidSyslog_Initialise(struct SolidSyslog* self, const struct SolidSyslogConfig* config);
void SolidSyslog_Cleanup(struct SolidSyslog* self);

/* No-op function-pointer defaults shared between SolidSyslog.c and
 * SolidSyslogStatic.c (the latter wires them into the exhaustion-fallback
 * NullInstance). No public Null equivalent exists for the function-pointer
 * typedefs, so they stay TU-internal across this class. */
void SolidSyslog_NullClock(struct SolidSyslogTimestamp* ts);
void SolidSyslog_NullHeaderField(struct SolidSyslogHeaderField* field, void* context);

static inline void SolidSyslog_Report(enum SolidSyslogSeverity severity, uint16_t category, enum SolidSyslogErrors code)
{
    SolidSyslog_Error(severity, &SolidSyslogErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGPRIVATE_H */

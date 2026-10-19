/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGPOSIXMESSAGEQUEUEBUFFERPRIVATE_H
#define SOLIDSYSLOGPOSIXMESSAGEQUEUEBUFFERPRIVATE_H

#include <stdint.h>

#include <mqueue.h>
#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogBufferDefinition.h"
#include "SolidSyslogDiscardPolicy.h"
#include "SolidSyslogError.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogPosixMessageQueueBufferErrors.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogTunables.h"

enum
{
    POSIX_MESSAGE_QUEUE_BUFFER_MAX_NAME_SIZE = 64
};

struct SolidSyslogMutex;

struct SolidSyslogPosixMessageQueueBuffer
{
    struct SolidSyslogBuffer Base;
    struct SolidSyslogMutex* Mutex;
    mqd_t Mq;
    SolidSyslogFormatterStorage
        NameStorage[SOLIDSYSLOG_FORMATTER_STORAGE_SIZE(POSIX_MESSAGE_QUEUE_BUFFER_MAX_NAME_SIZE)];
    size_t MaxMessageSize;
    SolidSyslogDiscardPolicyFunction GetDiscardPolicy;
    void* DiscardPolicyContext;
    /* Where the record Oldest takes out of the full queue lands; it is dropped
     * from here. Sized for any record SolidSyslog_Log formats. */
    char Evicted[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    /* Each report stands for a run of records the queue would not take as it
     * stood - full, or failing - or of sends that had to evict; the next record
     * it takes ends the first two, the next send that evicts nothing the
     * third. */
    bool RefusalReported;
    bool FailureReported;
    bool EvictionReported;
    /* The last send found the queue full - it evicted, or was refused - and
     * nothing has been received since: Log has outrun Service. Shared between
     * the two sides, so kept under the mutex. */
    bool Full;
};

bool SolidSyslogPosixMessageQueueBuffer_Initialise(
    struct SolidSyslogBuffer* base,
    struct SolidSyslogMutex* mutex,
    size_t maxMessageSize,
    size_t maxMessages,
    size_t slotIndex,
    SolidSyslogDiscardPolicyFunction getDiscardPolicy,
    void* discardPolicyContext
);
void SolidSyslogPosixMessageQueueBuffer_Cleanup(struct SolidSyslogBuffer* base);

static inline void PosixMessageQueueBuffer_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogPosixMessageQueueBufferErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogPosixMessageQueueBufferErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGPOSIXMESSAGEQUEUEBUFFERPRIVATE_H */

/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#include "SolidSyslogPendingEvents.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogBufferCategories.h"
#include "SolidSyslogError.h"
#include "SolidSyslogFileCategories.h"
#include "SolidSyslogSecurityPolicyCategories.h"
#include "SolidSyslogSenderCategories.h"
#include "SolidSyslogStoreCategories.h"
#include "SolidSyslogTlsStreamCategories.h"

struct PendingEventSlot
{
    const struct SolidSyslogErrorSource* Source;
    int32_t Detail;
    int32_t NativeCode;
    /* The event's position in PendingEvents_Severities plus one. Zero, where a
     * slot starts and where Clear returns it, stands for the kind's own
     * severity, so a slot read before its first event lands says nothing
     * more severe than its kind is raised at. */
    uint8_t SeverityCode;
    bool HasNativeCode;
    bool Pending;
};

/* A reported kind and the least severe severity it is raised at. */
struct PendingEventKind
{
    uint16_t Category;
    enum SolidSyslogSeverity Severity;
};

/* The reported kinds, in slot order. Index is identity: Take reports the
 * category from this table, so a slot never has to store it. The causes come
 * first, then the losses they lead to, then the delivery state, so a reader
 * walking the slots in order reports a cause ahead of what it caused: a file
 * fault ahead of the freeze, a refused handshake ahead of the failed delivery. */
static const struct PendingEventKind PendingEvents_Kinds[SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT] = {
    {SOLIDSYSLOG_CAT_FILE_IO_FAILED, SOLIDSYSLOG_SEVERITY_ERROR},
    {SOLIDSYSLOG_CAT_SECURITY_POLICY_KEY_UNAVAILABLE, SOLIDSYSLOG_SEVERITY_ERROR},
    {SOLIDSYSLOG_CAT_SECURITY_POLICY_SEAL_FAILED, SOLIDSYSLOG_SEVERITY_ERROR},
    {SOLIDSYSLOG_CAT_SECURITY_POLICY_OPEN_FAILED, SOLIDSYSLOG_SEVERITY_ERROR},
    {SOLIDSYSLOG_CAT_TLS_STREAM_HANDSHAKE_FAILED, SOLIDSYSLOG_SEVERITY_WARNING},
    {SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED, SOLIDSYSLOG_SEVERITY_ERROR},
    {SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST, SOLIDSYSLOG_SEVERITY_WARNING},
    {SOLIDSYSLOG_CAT_STORE_RECORDS_LOST, SOLIDSYSLOG_SEVERITY_WARNING},
    {SOLIDSYSLOG_CAT_STORE_RECORD_DISCARDED, SOLIDSYSLOG_SEVERITY_ERROR},
    {SOLIDSYSLOG_CAT_STORE_WRITE_FAILED, SOLIDSYSLOG_SEVERITY_ERROR},
    {SOLIDSYSLOG_CAT_STORE_FROZEN, SOLIDSYSLOG_SEVERITY_ERROR},
    {SOLIDSYSLOG_CAT_SENDER_RECORDS_LOST, SOLIDSYSLOG_SEVERITY_ERROR},
    {SOLIDSYSLOG_CAT_SENDER_DELIVERY_FAILED, SOLIDSYSLOG_SEVERITY_WARNING},
    {SOLIDSYSLOG_CAT_SENDER_DELIVERY_RESTORED, SOLIDSYSLOG_SEVERITY_NOTICE},
};

/* The severities in RFC 5424 order, which a slot's code indexes. */
static const enum SolidSyslogSeverity PendingEvents_Severities[] = {
    SOLIDSYSLOG_SEVERITY_EMERGENCY,
    SOLIDSYSLOG_SEVERITY_ALERT,
    SOLIDSYSLOG_SEVERITY_CRITICAL,
    SOLIDSYSLOG_SEVERITY_ERROR,
    SOLIDSYSLOG_SEVERITY_WARNING,
    SOLIDSYSLOG_SEVERITY_NOTICE,
    SOLIDSYSLOG_SEVERITY_INFORMATIONAL,
    SOLIDSYSLOG_SEVERITY_DEBUG,
};

static volatile struct PendingEventSlot PendingEvents_Slots[SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT];

static inline uint8_t PendingEvents_SeverityCode(enum SolidSyslogSeverity severity);
static inline enum SolidSyslogSeverity PendingEvents_SeverityOf(const struct PendingEventKind* kind, uint8_t code);

void SolidSyslogPendingEvents_Record(const struct SolidSyslogErrorEvent* event)
{
    size_t index = SolidSyslogPendingEvents_IndexOf(event->Category);
    if (index < SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT)
    {
        volatile struct PendingEventSlot* slot = &PendingEvents_Slots[index];
        /* Volatile stores, Pending last: the header states what that buys. */
        slot->SeverityCode = PendingEvents_SeverityCode(event->Severity);
        slot->Source = event->Source;
        slot->Detail = event->Detail;
        slot->NativeCode = event->NativeCode;
        slot->HasNativeCode = event->HasNativeCode;
        slot->Pending = true;
    }
}

/* A severity the table does not hold keeps code zero, the kind's own. */
static inline uint8_t PendingEvents_SeverityCode(enum SolidSyslogSeverity severity)
{
    const size_t severityCount = sizeof(PendingEvents_Severities) / sizeof(PendingEvents_Severities[0]);
    uint8_t code = 0U;
    for (size_t position = 0U; position < severityCount; position++)
    {
        if (PendingEvents_Severities[position] == severity)
        {
            code = (uint8_t) (position + 1U);
        }
    }
    return code;
}

size_t SolidSyslogPendingEvents_IndexOf(uint16_t category)
{
    size_t found = SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT;
    for (size_t index = 0U;
         (index < SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT) && (found == SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT);
         index++)
    {
        if (PendingEvents_Kinds[index].Category == category)
        {
            found = index;
        }
    }
    return found;
}

bool SolidSyslogPendingEvents_IsPending(size_t index)
{
    bool pending = false;
    if (index < SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT)
    {
        pending = PendingEvents_Slots[index].Pending;
    }
    return pending;
}

bool SolidSyslogPendingEvents_AnyPending(void)
{
    bool pending = false;
    for (size_t index = 0U; (index < SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT) && !pending; index++)
    {
        pending = PendingEvents_Slots[index].Pending;
    }
    return pending;
}

bool SolidSyslogPendingEvents_Take(size_t index, struct SolidSyslogErrorEvent* event)
{
    bool wasPending = false;
    if (index < SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT)
    {
        volatile struct PendingEventSlot* slot = &PendingEvents_Slots[index];
        wasPending = slot->Pending;
        slot->Pending = false;
        event->Severity = PendingEvents_SeverityOf(&PendingEvents_Kinds[index], slot->SeverityCode);
        event->Category = PendingEvents_Kinds[index].Category;
        event->HasNativeCode = slot->HasNativeCode;
        event->Source = slot->Source;
        event->Detail = slot->Detail;
        event->NativeCode = slot->NativeCode;
    }
    return wasPending;
}

/* A nonzero code comes only from PendingEvents_SeverityCode, so it is always
 * a position in the table plus one. */
static inline enum SolidSyslogSeverity PendingEvents_SeverityOf(const struct PendingEventKind* kind, uint8_t code)
{
    enum SolidSyslogSeverity severity = kind->Severity;
    if (code > 0U)
    {
        severity = PendingEvents_Severities[code - 1U];
    }
    return severity;
}

void SolidSyslogPendingEvents_Clear(void)
{
    for (size_t index = 0U; index < SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT; index++)
    {
        PendingEvents_Slots[index].Pending = false;
        PendingEvents_Slots[index].SeverityCode = 0U;
    }
}

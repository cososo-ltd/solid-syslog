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
    enum SolidSyslogSeverity Severity;
    int32_t Detail;
    int32_t NativeCode;
    bool HasNativeCode;
    bool Pending;
};

/* The reported kinds, in slot order. Index is identity: Take reports the
 * category from this table, so a slot never has to store it. */
static const uint16_t PendingEvents_Kinds[SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT] = {
    SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST,
    SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED,
    SOLIDSYSLOG_CAT_STORE_RECORDS_LOST,
    SOLIDSYSLOG_CAT_STORE_RECORD_DISCARDED,
    SOLIDSYSLOG_CAT_STORE_WRITE_FAILED,
    SOLIDSYSLOG_CAT_STORE_FROZEN,
    SOLIDSYSLOG_CAT_FILE_IO_FAILED,
    SOLIDSYSLOG_CAT_SENDER_RECORDS_LOST,
    SOLIDSYSLOG_CAT_SENDER_DELIVERY_FAILED,
    SOLIDSYSLOG_CAT_SENDER_DELIVERY_RESTORED,
    SOLIDSYSLOG_CAT_SECURITY_POLICY_KEY_UNAVAILABLE,
    SOLIDSYSLOG_CAT_SECURITY_POLICY_SEAL_FAILED,
    SOLIDSYSLOG_CAT_SECURITY_POLICY_OPEN_FAILED,
    SOLIDSYSLOG_CAT_TLS_STREAM_HANDSHAKE_FAILED,
};

static volatile struct PendingEventSlot PendingEvents_Slots[SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT];

static size_t PendingEvents_IndexOf(uint16_t category);

void SolidSyslogPendingEvents_Record(const struct SolidSyslogErrorEvent* event)
{
    size_t index = PendingEvents_IndexOf(event->Category);
    if (index < SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT)
    {
        volatile struct PendingEventSlot* slot = &PendingEvents_Slots[index];
        /* Volatile stores, Pending last: the header states what that buys. */
        slot->Severity = event->Severity;
        slot->Source = event->Source;
        slot->Detail = event->Detail;
        slot->NativeCode = event->NativeCode;
        slot->HasNativeCode = event->HasNativeCode;
        slot->Pending = true;
    }
}

/* The slot index of a category, or SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT for
 * a category that is not a reported kind. */
static size_t PendingEvents_IndexOf(uint16_t category)
{
    size_t found = SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT;
    for (size_t index = 0U;
         (index < SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT) && (found == SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT);
         index++)
    {
        if (PendingEvents_Kinds[index] == category)
        {
            found = index;
        }
    }
    return found;
}

bool SolidSyslogPendingEvents_Take(size_t index, struct SolidSyslogErrorEvent* event)
{
    bool wasPending = false;
    if (index < SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT)
    {
        volatile struct PendingEventSlot* slot = &PendingEvents_Slots[index];
        wasPending = slot->Pending;
        slot->Pending = false;
        event->Severity = slot->Severity;
        event->Category = PendingEvents_Kinds[index];
        event->HasNativeCode = slot->HasNativeCode;
        event->Source = slot->Source;
        event->Detail = slot->Detail;
        event->NativeCode = slot->NativeCode;
    }
    return wasPending;
}

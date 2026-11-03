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
    enum SolidSyslogSeverity Severity;
    const struct SolidSyslogErrorSource* Source;
    int32_t Detail;
    int32_t NativeCode;
    bool HasNativeCode;
    bool Pending;
};

static const uint16_t PENDING_EVENT_KINDS[SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT] = {
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

static struct PendingEventSlot pendingEventSlots[SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT];

void SolidSyslogPendingEvents_Reset(void)
{
    for (size_t index = 0U; index < SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT; index++)
    {
        pendingEventSlots[index].Pending = false;
    }
}

size_t SolidSyslogPendingEvents_IndexOf(uint16_t category)
{
    size_t found = SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT;
    for (size_t index = 0U; index < SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT; index++)
    {
        if (PENDING_EVENT_KINDS[index] == category)
        {
            found = index;
        }
    }
    return found;
}

void SolidSyslogPendingEvents_Record(const struct SolidSyslogErrorEvent* event)
{
    size_t index = SolidSyslogPendingEvents_IndexOf(event->Category);
    if (index < SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT)
    {
        struct PendingEventSlot* slot = &pendingEventSlots[index];
        slot->Severity = event->Severity;
        slot->Source = event->Source;
        slot->Detail = event->Detail;
        slot->NativeCode = event->NativeCode;
        slot->HasNativeCode = event->HasNativeCode;
        slot->Pending = true;
    }
}

bool SolidSyslogPendingEvents_Take(size_t index, struct SolidSyslogErrorEvent* event)
{
    bool wasPending = false;
    if (index < SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT)
    {
        struct PendingEventSlot* slot = &pendingEventSlots[index];
        wasPending = slot->Pending;
        slot->Pending = false;
        event->Severity = slot->Severity;
        event->Category = PENDING_EVENT_KINDS[index];
        event->HasNativeCode = slot->HasNativeCode;
        event->Source = slot->Source;
        event->Detail = slot->Detail;
        event->NativeCode = slot->NativeCode;
    }
    return wasPending;
}

/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGPENDINGEVENTS_H
#define SOLIDSYSLOGPENDINGEVENTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogExternC.h"

#define SOLIDSYSLOG_PENDING_EVENT_KIND_COUNT 14U

SOLIDSYSLOG_EXTERN_C_BEGIN

    void SolidSyslogPendingEvents_Reset(void);

    size_t SolidSyslogPendingEvents_IndexOf(uint16_t category);

    void SolidSyslogPendingEvents_Record(const struct SolidSyslogErrorEvent* event);

    bool SolidSyslogPendingEvents_Take(size_t index, struct SolidSyslogErrorEvent* event);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGPENDINGEVENTS_H */

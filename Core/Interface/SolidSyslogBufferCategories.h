/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Portable category constants (uint16_t macros) for the Buffer role:
 *  SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED and SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST. */
#ifndef SOLIDSYSLOGBUFFERCATEGORIES_H
#define SOLIDSYSLOGBUFFERCATEGORIES_H

#include <stdint.h>

#include "SolidSyslogErrorCategory.h"

/**
 * Portable Buffer-role error categories. Any Buffer implementation reuses
 * these; a portable handler switch on event->Category reacts to a buffer
 * backend failure identically regardless of the underlying mechanism
 * (POSIX message queue, ring...).
 */

/** The buffer's backend failed: a write or read the mechanism refused for a
 *  reason other than being full, or a head record too large for the reader.
 *  event->Source names which buffer, and event->Detail the fault. */
#define SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED ((uint16_t) (SOLIDSYSLOG_CAT_BUFFER_BASE + 1U))

/** The buffer lost records to its discard policy, working as configured: a
 *  full buffer refused the record being written - always under Newest and
 *  Halt, and under Oldest when it could not make room - or evicted records
 *  from its head to make room for it under Oldest. event->Detail says which
 *  happened, not which policy was in force. Raised as a WARNING, once per run: a run of refusals ends with the
 *  next record the buffer keeps, a run of evictions with the next write that
 *  evicts nothing. The logger raises it too, under its own source, when it
 *  drops the record it holds for the sender because the buffer filled behind
 *  it under Oldest. A full buffer clears as Service drains it, so a run
 *  usually means Service is falling behind the logging rate. */
#define SOLIDSYSLOG_CAT_BUFFER_RECORDS_LOST ((uint16_t) (SOLIDSYSLOG_CAT_BUFFER_BASE + 2U))

#endif /* SOLIDSYSLOGBUFFERCATEGORIES_H */

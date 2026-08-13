/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Portable category constants (uint16_t macros) for the Buffer role:
 *  SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED. */
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

/** The buffer's underlying backend refused a record (queue full, write failed,
 *  ...). event->Source names which buffer. */
#define SOLIDSYSLOG_CAT_BUFFER_BACKEND_FAILED ((uint16_t) (SOLIDSYSLOG_CAT_BUFFER_BASE + 1U))

#endif /* SOLIDSYSLOGBUFFERCATEGORIES_H */

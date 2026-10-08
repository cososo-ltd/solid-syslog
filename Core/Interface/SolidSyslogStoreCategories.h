/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Portable category constants (uint16_t macros) for the Store role:
 *  SOLIDSYSLOG_CAT_STORE_WRITE_FAILED and SOLIDSYSLOG_CAT_STORE_OPEN_FAILED. */
#ifndef SOLIDSYSLOGSTORECATEGORIES_H
#define SOLIDSYSLOGSTORECATEGORIES_H

#include <stdint.h>

#include "SolidSyslogErrorCategory.h"

/**
 * Portable Store-role error categories. They say what a fault cost the store;
 * whatever was at fault - the device or file beneath it, or the security
 * policy - reports the cause itself.
 */

/** The store could not keep a record: its device failed, or its security
 *  policy would not seal the record. event->Detail says which. The record is
 *  not retained. A record the discard policy turns away is not a failure and
 *  raises nothing. */
#define SOLIDSYSLOG_CAT_STORE_WRITE_FAILED ((uint16_t) (SOLIDSYSLOG_CAT_STORE_BASE + 1U))

/** The store could not open its device when it was created. It stands, and
 *  tries the device again on the next write. */
#define SOLIDSYSLOG_CAT_STORE_OPEN_FAILED ((uint16_t) (SOLIDSYSLOG_CAT_STORE_BASE + 2U))

#endif /* SOLIDSYSLOGSTORECATEGORIES_H */

/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Portable category constants (uint16_t macros) for the Store role:
 *  SOLIDSYSLOG_CAT_STORE_WRITE_FAILED and SOLIDSYSLOG_CAT_STORE_FROZEN. */
#ifndef SOLIDSYSLOGSTORECATEGORIES_H
#define SOLIDSYSLOGSTORECATEGORIES_H

#include <stdint.h>

#include "SolidSyslogErrorCategory.h"

/**
 * Portable Store-role error categories. They say what a fault cost the store;
 * whatever was at fault - the device or file beneath it, or the security
 * policy - reports the cause itself.
 */

/** The store could not keep a record because its security policy would not
 *  seal it. The record is not retained, and the policy reports why. A record
 *  the discard policy turns away is not a failure and raises nothing; a device
 *  that fails raises SOLIDSYSLOG_CAT_STORE_FROZEN instead. */
#define SOLIDSYSLOG_CAT_STORE_WRITE_FAILED ((uint16_t) (SOLIDSYSLOG_CAT_STORE_BASE + 1U))

/** The store's device failed, so the store will not use it again. event->Detail
 *  names the operation that failed. From then on the store keeps nothing and is
 *  transient, so Service sends each record straight to the sender; the records
 *  it held stay on the device, untouched, for the next start. Nothing recovers
 *  it short of destroying and creating it again. Raised once. */
#define SOLIDSYSLOG_CAT_STORE_FROZEN ((uint16_t) (SOLIDSYSLOG_CAT_STORE_BASE + 3U))

#endif /* SOLIDSYSLOGSTORECATEGORIES_H */

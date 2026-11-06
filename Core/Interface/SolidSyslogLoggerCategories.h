/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Portable category constants (uint16_t macros) for the logger itself:
 *  SOLIDSYSLOG_CAT_LOGGER_HALTED. */
#ifndef SOLIDSYSLOGLOGGERCATEGORIES_H
#define SOLIDSYSLOGLOGGERCATEGORIES_H

#include <stdint.h>

#include "SolidSyslogErrorCategory.h"

/** The logger halted: a queue refused a record while the discard policy
 *  answers Halt, and nothing moves until the policy answers otherwise.
 *  event->Detail says which queue. Raised as an ERROR, once per halt, just
 *  before OnHalt is called: an error handler that stops the device on an
 *  ERROR stops it here, and OnHalt is then never called. */
#define SOLIDSYSLOG_CAT_LOGGER_HALTED ((uint16_t) (SOLIDSYSLOG_CAT_LOGGER_BASE + 1U))

#endif /* SOLIDSYSLOGLOGGERCATEGORIES_H */

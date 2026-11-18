/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Portable category constants (uint16_t macros) for the NorFlash role:
 *  SOLIDSYSLOG_CAT_NOR_FLASH_IO_FAILED. */
#ifndef SOLIDSYSLOGNORFLASHCATEGORIES_H
#define SOLIDSYSLOGNORFLASHCATEGORIES_H

#include <stdint.h>

#include "SolidSyslogErrorCategory.h"

/**
 * Portable NorFlash-role error categories. Any NorFlash implementation reuses
 * these, and so does the block device that sits on one; a portable handler
 * switch on event->Category reacts to a failing flash identically whichever
 * part is beneath the store.
 */

/** A flash operation failed, or the flash did not hold what was programmed
 *  into it. event->Source says which class found it and event->Detail what it
 *  found. */
#define SOLIDSYSLOG_CAT_NOR_FLASH_IO_FAILED ((uint16_t) (SOLIDSYSLOG_CAT_NOR_FLASH_BASE + 1U))

#endif /* SOLIDSYSLOGNORFLASHCATEGORIES_H */

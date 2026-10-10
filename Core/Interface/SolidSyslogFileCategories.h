/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Portable category constants (uint16_t macros) for the File role:
 *  SOLIDSYSLOG_CAT_FILE_IO_FAILED. */
#ifndef SOLIDSYSLOGFILECATEGORIES_H
#define SOLIDSYSLOGFILECATEGORIES_H

#include <stdint.h>

#include "SolidSyslogErrorCategory.h"

/**
 * Portable File-role error categories. Any File implementation reuses these; a
 * portable handler switch on event->Category reacts to a failing file system
 * identically whichever backend sits beneath the store.
 */

/** A file system call failed. event->Detail is the enum SolidSyslogFileErrors
 *  code naming the operation, and a SOLIDSYSLOG_CAT_NATIVE_ERROR event follows
 *  when the platform gave a code. */
#define SOLIDSYSLOG_CAT_FILE_IO_FAILED ((uint16_t) (SOLIDSYSLOG_CAT_FILE_BASE + 1U))

#endif /* SOLIDSYSLOGFILECATEGORIES_H */

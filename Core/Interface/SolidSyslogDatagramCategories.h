/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Portable category constants (uint16_t macros) for the Datagram role:
 *  SOLIDSYSLOG_CAT_DATAGRAM_NEXT_HOP_UNRESOLVED. */
#ifndef SOLIDSYSLOGDATAGRAMCATEGORIES_H
#define SOLIDSYSLOGDATAGRAMCATEGORIES_H

#include <stdint.h>

#include "SolidSyslogErrorCategory.h"

/**
 * Portable Datagram-role error categories. Any Datagram implementation reuses
 * these; a portable handler switch on event->Category reacts identically
 * whichever backend sends the record.
 */

/** The link address of the next hop to the destination did not resolve within
 *  the wait, so the record was not handed to the stack. Raised once when
 *  resolution starts failing, not for every record while it fails.
 *  event->Detail is SOLIDSYSLOG_DATAGRAM_ERROR_NEXT_HOP_UNRESOLVED, and a
 *  SOLIDSYSLOG_CAT_NATIVE_ERROR event follows when the platform gave a code. */
#define SOLIDSYSLOG_CAT_DATAGRAM_NEXT_HOP_UNRESOLVED ((uint16_t) (SOLIDSYSLOG_CAT_DATAGRAM_BASE + 1U))

#endif /* SOLIDSYSLOGDATAGRAMCATEGORIES_H */

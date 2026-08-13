/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  Portable category constants (uint16_t macros) for the TLS-stream role:
 *  SOLIDSYSLOG_CAT_TLS_STREAM_INIT_FAILED / _HANDSHAKE_FAILED. */
#ifndef SOLIDSYSLOGTLSSTREAMCATEGORIES_H
#define SOLIDSYSLOGTLSSTREAMCATEGORIES_H

#include <stdint.h>

#include "SolidSyslogErrorCategory.h"

/**
 * Portable TLS-stream error categories, shared by every TLS backend (OpenSSL,
 * Mbed TLS, or an integrator's own). A portable handler switch on
 * event->Category reacts to a TLS init or handshake failure identically
 * regardless of which TLS library produced it.
 */

/** TLS context / library setup failed before any handshake (bad CA bundle,
 *  cert or key load failure...). */
#define SOLIDSYSLOG_CAT_TLS_STREAM_INIT_FAILED ((uint16_t) (SOLIDSYSLOG_CAT_TLS_STREAM_BASE + 1U))
/** The TLS handshake with the server did not complete (peer verification,
 *  protocol, or transport failure). */
#define SOLIDSYSLOG_CAT_TLS_STREAM_HANDSHAKE_FAILED ((uint16_t) (SOLIDSYSLOG_CAT_TLS_STREAM_BASE + 2U))

#endif /* SOLIDSYSLOGTLSSTREAMCATEGORIES_H */

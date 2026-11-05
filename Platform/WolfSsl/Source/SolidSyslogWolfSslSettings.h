/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/* The integrator's wolfSSL build options, before any wolfSSL header. A
 * ./configure build describes itself in options.h; a user_settings.h build
 * defines WOLFSSL_USER_SETTINGS and settings.h reads that file instead. Every
 * translation unit in this pack includes this header first, so the pack sees
 * the build it is compiled against whichever way it was made. */
#ifndef SOLIDSYSLOGWOLFSSLSETTINGS_H
#define SOLIDSYSLOGWOLFSSLSETTINGS_H

#ifndef WOLFSSL_USER_SETTINGS
#include <wolfssl/options.h>
#endif
#include <wolfssl/wolfcrypt/settings.h>

/* How a pin is checked where trust anchors are installed too. wolfSSL calls
 * the verify callback on a leaf that verifies only under
 * WOLFSSL_ALWAYS_VERIFY_CB, so without it a chain that verifies never reaches
 * the pin; KEEP_PEER_CERT keeps the leaf for a check once the handshake is
 * done; with neither, the combination is refused. Each choice is one body,
 * in its own translation unit. A build may choose a later one than its flags
 * allow by defining SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK itself, and is
 * stopped here if it asks for one its flags cannot honour. */
#define SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK_IN_CALLBACK 1
#define SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK_AFTER_HANDSHAKE 2
#define SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK_REFUSED 3

#ifndef SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK
#if defined(WOLFSSL_ALWAYS_VERIFY_CB)
#define SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK_IN_CALLBACK
#elif defined(KEEP_PEER_CERT)
#define SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK_AFTER_HANDSHAKE
#else
#define SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK_REFUSED
#endif
#endif

#if (SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK == SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK_IN_CALLBACK) && \
    !defined(WOLFSSL_ALWAYS_VERIFY_CB)
#error "SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK asks for the in-callback check, which needs WOLFSSL_ALWAYS_VERIFY_CB."
#endif
#if (SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK == SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK_AFTER_HANDSHAKE) && \
    !defined(KEEP_PEER_CERT)
#error "SOLIDSYSLOG_WOLFSSL_ANCHORED_PIN_CHECK asks for the after-handshake check, which needs KEEP_PEER_CERT."
#endif

#endif /* SOLIDSYSLOGWOLFSSLSETTINGS_H */

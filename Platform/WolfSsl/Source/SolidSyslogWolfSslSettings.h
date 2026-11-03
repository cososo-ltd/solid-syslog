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

#endif /* SOLIDSYSLOGWOLFSSLSETTINGS_H */

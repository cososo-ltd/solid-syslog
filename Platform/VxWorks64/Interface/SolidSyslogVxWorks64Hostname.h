/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The VxWorks 6.4 hostname SolidSyslogHeaderFieldFunction, for
 *  SolidSyslogConfig.GetHostname. */
#ifndef SOLIDSYSLOGVXWORKS64HOSTNAME_H
#define SOLIDSYSLOGVXWORKS64HOSTNAME_H

#include "SolidSyslogExternC.h"

struct SolidSyslogHeaderField;

SOLIDSYSLOG_EXTERN_C_BEGIN

    void SolidSyslogVxWorks64_GetHostname(struct SolidSyslogHeaderField * field, void* context);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGVXWORKS64HOSTNAME_H */

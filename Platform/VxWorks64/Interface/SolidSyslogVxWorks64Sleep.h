/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The VxWorks 6.4 SolidSyslogSleepFunction. */
#ifndef SOLIDSYSLOGVXWORKS64SLEEP_H
#define SOLIDSYSLOGVXWORKS64SLEEP_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Sleeps for @p milliseconds via taskDelay, rounded up to whole system
     *  clock ticks. Zero or a negative value yields without sleeping. It is for
     *  the short waits a caller makes between its own bounded attempts, such as
     *  the TLS handshake's: a sleep of up to one minute converts to ticks
     *  without overflow at any tick rate below 35 MHz, and a longer one is outside this
     *  contract. It neither performs nor bounds retries. */
    void SolidSyslogVxWorks64_Sleep(int milliseconds);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGVXWORKS64SLEEP_H */

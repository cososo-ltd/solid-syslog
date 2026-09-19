/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The uptime conversion behind SolidSyslogFreeRtos_GetSysUpTime, separated
 *  from the tick source and the tick rate so it can be driven at rates the
 *  test build is not compiled at. Not an integration surface - the public
 *  entry point is SolidSyslogFreeRtosSysUpTime.h. */
#ifndef SOLIDSYSLOGFREERTOSSYSUPTIMEPRIVATE_H
#define SOLIDSYSLOGFREERTOSSYSUPTIMEPRIVATE_H

#include "SolidSyslogExternC.h"

#include <stdint.h>

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** How far the tick counter has run beyond its own range. A 32-bit
     *  TickType_t rolls over long before 2^32 hundredths do, and the counter
     *  cannot say how often it has, so the phase is carried here. */
    struct SolidSyslogFreeRtosSysUpTimeState
    {
        uint32_t LastTicks;
        uint32_t Rollovers;
    };

    /** Fold @p nowTicks into @p state and return the uptime in hundredths of
     *  a second, wrapping at 2^32 as RFC 3418 TimeTicks does.
     *
     *  A tick count below the last one seen is taken as one rollover, so this
     *  has to be called at least once per rollover period - about 50 days at
     *  1000 Hz on a 32-bit counter - or an unobserved rollover is lost. Every
     *  formatted message calls it, so silence that long is the only way to
     *  reach it. */
    uint32_t SolidSyslogFreeRtosSysUpTime_Advance(
        struct SolidSyslogFreeRtosSysUpTimeState * state,
        uint32_t nowTicks,
        uint32_t tickRateHz
    );

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGFREERTOSSYSUPTIMEPRIVATE_H */

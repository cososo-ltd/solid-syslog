/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The stack scratch: SolidSyslog_Log formats each message in a buffer of
 *  SOLIDSYSLOG_MAX_MESSAGE_SIZE on the stack of the task that logs. Lock-free,
 *  and the default when SolidSyslogConfig.Scratch is NULL. Every task that logs
 *  must have that much stack to spare; where several tasks log and RAM is
 *  tight, SolidSyslogSharedScratch spends the buffer once instead. */
#ifndef SOLIDSYSLOGSTACKSCRATCH_H
#define SOLIDSYSLOGSTACKSCRATCH_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogScratch;

    /** The shared stateless instance; the same pointer on every call. */
    struct SolidSyslogScratch* SolidSyslogStackScratch_Get(void);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGSTACKSCRATCH_H */

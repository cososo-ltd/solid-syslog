/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The capture state: whether the device has decided that security logging
 *  is on, and the function an integrator injects to answer it. */
#ifndef SOLIDSYSLOGCAPTURESTATE_H
#define SOLIDSYSLOGCAPTURESTATE_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Whether the device has decided that security logging is on. Undecided
     *  is the state before its configuration is read: records are held in
     *  RAM. On captures them. Off is the user's opt-out: a record is dropped
     *  as it is logged, and records held while undecided are discarded; what
     *  was captured while on is still delivered. */
    enum SolidSyslogCaptureState
    {
        SOLIDSYSLOG_CAPTURE_UNDECIDED,
        SOLIDSYSLOG_CAPTURE_ON,
        SOLIDSYSLOG_CAPTURE_OFF
    };

    /** Answers the device's capture state. The logger asks at the moment it
     *  has to decide, so the answer may change on a running device: from the
     *  task that logs, in SolidSyslog_Log, and from the task that calls
     *  SolidSyslog_Service. The context is the one paired with it, passed
     *  back unchanged. */
    typedef enum SolidSyslogCaptureState (*SolidSyslogCaptureStateFunction)(void* context);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGCAPTURESTATE_H */

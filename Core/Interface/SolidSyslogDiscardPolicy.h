/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The discard policy: what a full queue does with the record that does not
 *  fit, and the function an integrator injects to answer which policy applies. */
#ifndef SOLIDSYSLOGDISCARDPOLICY_H
#define SOLIDSYSLOGDISCARDPOLICY_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** What a full queue does with the record that does not fit. Oldest makes
     *  room by dropping the oldest records it holds; Newest refuses the record
     *  being written; Halt refuses it too, and the logger stops moving records
     *  until the policy is changed or the device restarts. Records lost either way
     *  are reported by the queue that lost them, once per run. */
    enum SolidSyslogDiscardPolicy
    {
        SOLIDSYSLOG_DISCARD_POLICY_OLDEST,
        SOLIDSYSLOG_DISCARD_POLICY_NEWEST,
        SOLIDSYSLOG_DISCARD_POLICY_HALT
    };

    /** Answers which policy applies. A queue asks at the moment it has to decide,
     *  so the answer may change on a running device. The store and Service ask
     *  from the task that calls SolidSyslog_Service. The context is the one
     *  paired with it, passed back unchanged. */
    typedef enum SolidSyslogDiscardPolicy (*SolidSyslogDiscardPolicyFunction)(void* context);

    /** The policy applied where none was injected: Oldest. */
    enum SolidSyslogDiscardPolicy SolidSyslogDiscardPolicy_Default(void* context);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGDISCARDPOLICY_H */

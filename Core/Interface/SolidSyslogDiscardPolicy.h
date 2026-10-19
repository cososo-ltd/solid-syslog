/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The discard policy: what a full queue does with the record that does not
 *  fit. One policy covers every queue a record passes through on its way to
 *  the collector, the buffer and the store alike. */
#ifndef SOLIDSYSLOGDISCARDPOLICY_H
#define SOLIDSYSLOGDISCARDPOLICY_H

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

#endif /* SOLIDSYSLOGDISCARDPOLICY_H */

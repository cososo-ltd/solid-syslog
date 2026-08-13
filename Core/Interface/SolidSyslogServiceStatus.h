/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The advisory servicing-hint enum SolidSyslog_Service returns to drive a
 *  host loop (idle / ready / blocked / halted). */
#ifndef SOLIDSYSLOGSERVICESTATUS_H
#define SOLIDSYSLOGSERVICESTATUS_H

/** Advisory servicing hint returned by SolidSyslog_Service, which documents
 *  each state and the host action it suggests. */
enum SolidSyslogServiceStatus
{
    SOLIDSYSLOG_SERVICE_IDLE,
    SOLIDSYSLOG_SERVICE_READY,
    SOLIDSYSLOG_SERVICE_BLOCKED,
    SOLIDSYSLOG_SERVICE_HALTED
};

#endif /* SOLIDSYSLOGSERVICESTATUS_H */

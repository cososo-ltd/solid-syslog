/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

/** @file
 *  The callback typedef the integrator installs on a Datagram config to state
 *  the IPv4 payload limit where the stack cannot discover the path MTU
 *  (runtime-tunable). */
#ifndef SOLIDSYSLOGUDPPAYLOADLIMITFUNCTION_H
#define SOLIDSYSLOGUDPPAYLOADLIMITFUNCTION_H

#include <stddef.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    /** Returns the largest IPv4 UDP payload the path carries unfragmented, in
     *  bytes - SolidSyslogUdpPayload_FromMtu(1500, false) on a plain Ethernet
     *  path - or zero to leave it to the SOLIDSYSLOG_UDP_IPV4_PAYLOAD_LIMIT
     *  tunable. Never guess high: the sender trims an oversize record to this
     *  and retries, so a figure the path cannot carry loses the record. Called
     *  on every MaxPayload query, so a runtime-tunable value (a commissioning-
     *  time setting, a live override) takes effect on the next query without
     *  destroying the datagram. @p context is passed through unchanged from
     *  the config slot the integrator installed it on. */
    typedef size_t (*SolidSyslogUdpPayloadLimitFunction)(void* context);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGUDPPAYLOADLIMITFUNCTION_H */

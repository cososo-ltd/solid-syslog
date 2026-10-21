/* SPDX-FileCopyrightText: Copyright 2026 Cozens Software Solutions Limited
 * SPDX-License-Identifier: PolyForm-Noncommercial-1.0.0 OR LicenseRef-PolyForm-Internal-Use-1.0.0 OR LicenseRef-COSOSO-Commercial
 */

#ifndef SOLIDSYSLOGLWIPRAWADDRESSPRIVATE_H
#define SOLIDSYSLOGLWIPRAWADDRESSPRIVATE_H

#include <stdbool.h>
#include <stdint.h>

#include "SolidSyslogError.h"
#include "SolidSyslogLwipRawAddressErrors.h"
#include "SolidSyslogPrival.h"
#include "lwip/ip_addr.h"

struct SolidSyslogAddress;

/* POD payload behind the opaque SolidSyslogAddress handle. lwIP carries
 * the destination IP and port as independent arguments to udp_sendto /
 * tcp_connect, so we store them as independent fields and let consumers
 * read/write them directly via -> on the downcast pointer. No per-field
 * accessors - there is no invariant to enforce. */
struct SolidSyslogLwipRawAddress
{
    ip_addr_t Ip;
    u16_t Port;
};

void SolidSyslogLwipRawAddress_Initialise(struct SolidSyslogAddress* base);
void SolidSyslogLwipRawAddress_Cleanup(struct SolidSyslogAddress* base);

static inline struct SolidSyslogLwipRawAddress* SolidSyslogLwipRawAddress_As(struct SolidSyslogAddress* base)
{
    return (struct SolidSyslogLwipRawAddress*) base;
}

static inline const struct SolidSyslogLwipRawAddress* SolidSyslogLwipRawAddress_AsConst(
    const struct SolidSyslogAddress* base
)
{
    return (const struct SolidSyslogLwipRawAddress*) base;
}

/* The family the pack's transports send to. A dual-stack lwIP parses and
 * resolves IPv6 too; on an IPv4-only one IP_IS_V4 is the constant 1 and the
 * address goes unread. */
static inline bool LwipRawAddress_IsIpv4(const ip_addr_t* address)
{
    (void) address;
    return (IP_IS_V4(address) != 0);
}

static inline void LwipRawAddress_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogAddressErrors code
)
{
    SolidSyslog_Error(severity, &SolidSyslogLwipRawAddressErrorSource, category, (int32_t) code);
}

#endif /* SOLIDSYSLOGLWIPRAWADDRESSPRIVATE_H */

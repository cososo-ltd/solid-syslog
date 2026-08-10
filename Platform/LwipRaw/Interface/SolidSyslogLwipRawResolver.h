/** @file
 *  The lwIP Raw resolver for a collector named by address: it resolves nothing,
 *  it parses the numeric literal you configured.
 *
 *  Resolve delegates to lwIP's ipaddr_aton to parse the endpoint host as a
 *  numeric IP literal, writing it into the destination SolidSyslogAddress;
 *  whatever ipaddr_aton accepts is accepted, whatever it rejects (DNS names,
 *  the empty string, and other non-address text) fails the Resolve, so the caller's
 *  unresolved-host error path runs. The transport is ignored. The parse touches
 *  no lwIP core state, so it takes no marshal hop.
 *
 *  Choosing between the two: this one where the endpoint is always an address,
 *  since it needs neither LWIP_DNS nor a Sleep;
 *  SolidSyslogLwipRawDnsResolver.h where it may be a name, which resolves names
 *  and addresses alike. A build wires whichever one its deployment needs. */
#ifndef SOLIDSYSLOGLWIPRAWRESOLVER_H
#define SOLIDSYSLOGLWIPRAWRESOLVER_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogResolver;

    /** Create takes no config; an exhausted pool falls back to the shared
     *  NullResolver. */
    struct SolidSyslogResolver* SolidSyslogLwipRawResolver_Create(void);
    /** Release the pool slot. */
    void SolidSyslogLwipRawResolver_Destroy(struct SolidSyslogResolver * base);

SOLIDSYSLOG_EXTERN_C_END

#endif /* SOLIDSYSLOGLWIPRAWRESOLVER_H */

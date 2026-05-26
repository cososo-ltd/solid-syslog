#ifndef SOLIDSYSLOGLWIPRAWADDRESSPRIVATE_H
#define SOLIDSYSLOGLWIPRAWADDRESSPRIVATE_H

#include "lwip/ip_addr.h"

struct SolidSyslogAddress;

/* POD payload behind the opaque SolidSyslogAddress handle. lwIP carries
 * the destination IP and port as independent arguments to udp_sendto /
 * tcp_connect, so we store them as independent fields and let consumers
 * read/write them directly via -> on the downcast pointer. No per-field
 * accessors — there is no invariant to enforce. */
struct SolidSyslogLwipRawAddress
{
    ip_addr_t Ip;
    u16_t Port;
};

void LwipRawAddress_Initialise(struct SolidSyslogAddress* base);
void LwipRawAddress_Cleanup(struct SolidSyslogAddress* base);

static inline struct SolidSyslogLwipRawAddress* SolidSyslogLwipRawAddress_As(struct SolidSyslogAddress* base)
{
    // cppcheck-suppress cstyleCast -- opaque-to-impl downcast: pool slot is a real SolidSyslogLwipRawAddress
    return (struct SolidSyslogLwipRawAddress*) base;
}

static inline const struct SolidSyslogLwipRawAddress* SolidSyslogLwipRawAddress_AsConst(
    const struct SolidSyslogAddress* base
)
{
    // cppcheck-suppress cstyleCast -- opaque-to-impl downcast: pool slot is a real SolidSyslogLwipRawAddress
    return (const struct SolidSyslogLwipRawAddress*) base;
}

#endif /* SOLIDSYSLOGLWIPRAWADDRESSPRIVATE_H */

#include "BddTargetIps.h"
#include "SolidSyslogSdValue.h"

enum
{
    BDD_TARGET_IP_MAX = 64 /* matches ORIGIN_IP_MAX in OriginSd */
};

/* Static demo IP — in real deployments this comes from getifaddrs(3) on POSIX,
   GetAdaptersAddresses on Windows, or wherever the host's reachable addresses
   are observed. The library supplies the callback shape; address enumeration
   is opinionated and left to integrators.

   The example wires a single IP. The library happily emits multiple `ip="..."`
   per RFC 5424 §7.2 (proven by SolidSyslogOriginSdTest's
   FormatIncludesMultipleIpsFromCallback) but syslog-ng's ${SDATA} macro
   exposes the parsed-and-deduplicated view, so a multi-IP example would not
   roundtrip cleanly through the syslog-ng BDD oracle. */
static const char* const BDD_TARGET_IPS[] = {"192.0.2.1"};

size_t BddTargetIps_Count(void* context)
{
    (void) context;
    return sizeof(BDD_TARGET_IPS) / sizeof(BDD_TARGET_IPS[0]);
}

void BddTargetIps_At(struct SolidSyslogSdValue* value, void* context, size_t index)
{
    (void) context;
    SolidSyslogSdValue_BoundedString(value, BDD_TARGET_IPS[index], BDD_TARGET_IP_MAX);
}

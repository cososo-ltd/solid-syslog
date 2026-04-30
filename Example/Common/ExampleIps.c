#include "ExampleIps.h"
#include "SolidSyslogFormatter.h"

enum
{
    EXAMPLE_IP_MAX = 64 /* matches ORIGIN_IP_MAX in OriginSd */
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
static const char* const EXAMPLE_IPS[] = {"192.0.2.1"};

size_t ExampleIps_Count(void)
{
    return sizeof(EXAMPLE_IPS) / sizeof(EXAMPLE_IPS[0]);
}

void ExampleIps_At(struct SolidSyslogFormatter* formatter, size_t index)
{
    SolidSyslogFormatter_EscapedString(formatter, EXAMPLE_IPS[index], EXAMPLE_IP_MAX);
}

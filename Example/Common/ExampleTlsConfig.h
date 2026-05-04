#ifndef EXAMPLETLSCONFIG_H
#define EXAMPLETLSCONFIG_H

#include <stdint.h>

#include "ExternC.h"

struct SolidSyslogEndpoint;

EXTERN_C_BEGIN

    const char* ExampleTlsConfig_GetHost(void);
    uint16_t    ExampleTlsConfig_GetPort(void);
    const char* ExampleTlsConfig_GetCaBundlePath(void);
    const char* ExampleTlsConfig_GetServerName(void);
    void        ExampleTlsConfig_GetEndpoint(struct SolidSyslogEndpoint * endpoint);
    uint32_t    ExampleTlsConfig_GetEndpointVersion(void);

    /* Override the default TLS host ("syslog-ng" — Linux compose service
       name). Caller owns the string lifetime. Used by the per-platform
       main.c to inject SOLIDSYSLOG_BDD_TLS_HOST when set, so the same
       example targets the Linux compose oracle or the Windows OTel oracle
       on 127.0.0.1. */
    void ExampleTlsConfig_SetHost(const char* host);

EXTERN_C_END

#endif /* EXAMPLETLSCONFIG_H */

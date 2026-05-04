#ifndef EXAMPLEMTLSCONFIG_H
#define EXAMPLEMTLSCONFIG_H

#include <stdint.h>

#include "ExternC.h"

struct SolidSyslogEndpoint;

EXTERN_C_BEGIN

    const char* ExampleMtlsConfig_GetHost(void);
    uint16_t    ExampleMtlsConfig_GetPort(void);
    const char* ExampleMtlsConfig_GetCaBundlePath(void);
    const char* ExampleMtlsConfig_GetServerName(void);
    const char* ExampleMtlsConfig_GetClientCertChainPath(void);
    const char* ExampleMtlsConfig_GetClientKeyPath(void);
    void        ExampleMtlsConfig_GetEndpoint(struct SolidSyslogEndpoint * endpoint);
    uint32_t    ExampleMtlsConfig_GetEndpointVersion(void);

    /* Override the default mTLS host ("syslog-ng" — Linux compose service
       name). Caller owns the string lifetime. Used by per-platform main.c
       to inject SOLIDSYSLOG_BDD_MTLS_HOST when set. */
    void ExampleMtlsConfig_SetHost(const char* host);

EXTERN_C_END

#endif /* EXAMPLEMTLSCONFIG_H */

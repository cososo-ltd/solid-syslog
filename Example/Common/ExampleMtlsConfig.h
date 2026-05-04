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

EXTERN_C_END

#endif /* EXAMPLEMTLSCONFIG_H */

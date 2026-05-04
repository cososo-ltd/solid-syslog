#ifndef EXAMPLETCPCONFIG_H
#define EXAMPLETCPCONFIG_H

#include <stdint.h>

#include "ExternC.h"

struct SolidSyslogEndpoint;

EXTERN_C_BEGIN

    const char* ExampleTcpConfig_GetHost(void);
    uint16_t    ExampleTcpConfig_GetPort(void);
    void        ExampleTcpConfig_GetEndpoint(struct SolidSyslogEndpoint * endpoint);
    uint32_t    ExampleTcpConfig_GetEndpointVersion(void);

EXTERN_C_END

#endif /* EXAMPLETCPCONFIG_H */

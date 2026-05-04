#ifndef EXAMPLEUDPCONFIG_H
#define EXAMPLEUDPCONFIG_H

#include <stdint.h>

#include "ExternC.h"

struct SolidSyslogEndpoint;

EXTERN_C_BEGIN

    const char* ExampleUdpConfig_GetHost(void);
    uint16_t    ExampleUdpConfig_GetPort(void);
    void        ExampleUdpConfig_GetEndpoint(struct SolidSyslogEndpoint * endpoint);
    uint32_t    ExampleUdpConfig_GetEndpointVersion(void);

EXTERN_C_END

#endif /* EXAMPLEUDPCONFIG_H */

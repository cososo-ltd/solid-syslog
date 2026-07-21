#ifndef BDDTARGETTCPCONFIG_H
#define BDDTARGETTCPCONFIG_H

#include <stdint.h>

#include "ExternC.h"

struct SolidSyslogEndpoint;

EXTERN_C_BEGIN

    const char* BddTargetTcpConfig_GetHost(void);
    uint16_t BddTargetTcpConfig_GetPort(void);
    void BddTargetTcpConfig_GetEndpoint(struct SolidSyslogEndpoint * endpoint, void* context);
    uint32_t BddTargetTcpConfig_GetEndpointVersion(void* context);

EXTERN_C_END

#endif /* BDDTARGETTCPCONFIG_H */

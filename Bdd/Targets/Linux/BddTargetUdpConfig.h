#ifndef BDDTARGETUDPCONFIG_H
#define BDDTARGETUDPCONFIG_H

#include <stdint.h>

#include "ExternC.h"

struct SolidSyslogEndpoint;

EXTERN_C_BEGIN

    const char* BddTargetUdpConfig_GetHost(void);
    uint16_t BddTargetUdpConfig_GetPort(void);
    void BddTargetUdpConfig_GetEndpoint(struct SolidSyslogEndpoint * endpoint, void* context);
    uint32_t BddTargetUdpConfig_GetEndpointVersion(void* context);

EXTERN_C_END

#endif /* BDDTARGETUDPCONFIG_H */

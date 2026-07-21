#include "BddTargetUdpConfig.h"

#include <stdint.h>

#include "SolidSyslogEndpointHost.h"
#include "SolidSyslogEndpoint.h"

/* Unprivileged mirror of SOLIDSYSLOG_UDP_DEFAULT_PORT (514) for BDD containers */
enum
{
    BDD_TARGET_UDP_PORT = 5514
};

const char* BddTargetUdpConfig_GetHost(void)
{
    return "syslog-ng";
}

uint16_t BddTargetUdpConfig_GetPort(void)
{
    return (uint16_t) BDD_TARGET_UDP_PORT;
}

void BddTargetUdpConfig_GetEndpoint(struct SolidSyslogEndpoint* endpoint, void* context)
{
    (void) context;
    SolidSyslogEndpointHost_String(endpoint->Host, BddTargetUdpConfig_GetHost(), SOLIDSYSLOG_MAX_HOST_SIZE);
    endpoint->Port = BddTargetUdpConfig_GetPort();
}

/* Static config — host/port never change, so version stays 0 forever and the
   sender connects exactly once. */
uint32_t BddTargetUdpConfig_GetEndpointVersion(void* context)
{
    (void) context;
    return 0;
}

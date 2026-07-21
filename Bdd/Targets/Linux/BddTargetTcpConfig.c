#include "BddTargetTcpConfig.h"

#include <stdint.h>

#include "SolidSyslogEndpointHost.h"
#include "SolidSyslogEndpoint.h"

/* Unprivileged port used by the BDD syslog-ng container — library default is
   SOLIDSYSLOG_TCP_DEFAULT_PORT (601, RFC 6587 §3.2 / IANA) which requires root. */
enum
{
    BDD_TARGET_TCP_PORT = 5514
};

const char* BddTargetTcpConfig_GetHost(void)
{
    return "syslog-ng";
}

uint16_t BddTargetTcpConfig_GetPort(void)
{
    return (uint16_t) BDD_TARGET_TCP_PORT;
}

void BddTargetTcpConfig_GetEndpoint(struct SolidSyslogEndpoint* endpoint, void* context)
{
    (void) context;
    SolidSyslogEndpointHost_String(endpoint->Host, BddTargetTcpConfig_GetHost(), SOLIDSYSLOG_MAX_HOST_SIZE);
    endpoint->Port = BddTargetTcpConfig_GetPort();
}

/* Static config — host/port never change, so version stays 0 forever and the
   sender connects exactly once. */
uint32_t BddTargetTcpConfig_GetEndpointVersion(void* context)
{
    (void) context;
    return 0;
}

#include "ExampleUdpConfig.h"

#include <stdint.h>

#include "SolidSyslogFormatter.h"
#include "SolidSyslogEndpoint.h"

/* Unprivileged mirror of SOLIDSYSLOG_UDP_DEFAULT_PORT (514) for BDD containers */
enum
{
    EXAMPLE_UDP_PORT = 5514
};

const char* ExampleUdpConfig_GetHost(void)
{
    return "syslog-ng";
}

uint16_t ExampleUdpConfig_GetPort(void)
{
    return (uint16_t) EXAMPLE_UDP_PORT;
}

void ExampleUdpConfig_GetEndpoint(struct SolidSyslogEndpoint* endpoint)
{
    SolidSyslogFormatter_BoundedString(endpoint->host, ExampleUdpConfig_GetHost(), SOLIDSYSLOG_MAX_HOST_SIZE);
    endpoint->port = ExampleUdpConfig_GetPort();
}

/* Static config — host/port never change, so version stays 0 forever and the
   sender connects exactly once. */
uint32_t ExampleUdpConfig_GetEndpointVersion(void)
{
    return 0;
}

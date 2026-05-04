#include "ExampleTcpConfig.h"

#include <stdint.h>

#include "SolidSyslogFormatter.h"
#include "SolidSyslogEndpoint.h"

/* Unprivileged port used by the BDD syslog-ng container — library default is
   SOLIDSYSLOG_TCP_DEFAULT_PORT (601, RFC 6587 §3.2 / IANA) which requires root. */
enum
{
    EXAMPLE_TCP_PORT = 5514
};

const char* ExampleTcpConfig_GetHost(void)
{
    return "syslog-ng";
}

uint16_t ExampleTcpConfig_GetPort(void)
{
    return (uint16_t) EXAMPLE_TCP_PORT;
}

void ExampleTcpConfig_GetEndpoint(struct SolidSyslogEndpoint* endpoint)
{
    SolidSyslogFormatter_BoundedString(endpoint->host, ExampleTcpConfig_GetHost(), SOLIDSYSLOG_MAX_HOST_SIZE);
    endpoint->port = ExampleTcpConfig_GetPort();
}

/* Static config — host/port never change, so version stays 0 forever and the
   sender connects exactly once. */
uint32_t ExampleTcpConfig_GetEndpointVersion(void)
{
    return 0;
}

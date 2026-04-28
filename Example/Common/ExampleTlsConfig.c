#include "ExampleTlsConfig.h"
#include "SolidSyslogFormatter.h"
#include "SolidSyslogTransport.h"

#include <stdint.h>

/* Test CA for BDD. Paths are relative to the working directory the example is
 * launched from (/workspaces/SolidSyslog in the BDD container). */
static const char* const EXAMPLE_TLS_CA_BUNDLE_PATH = "Bdd/syslog-ng/tls/ca.pem";

const char* ExampleTlsConfig_GetHost(void)
{
    return "syslog-ng";
}

uint16_t ExampleTlsConfig_GetPort(void)
{
    return (uint16_t) SOLIDSYSLOG_TLS_DEFAULT_PORT;
}

const char* ExampleTlsConfig_GetCaBundlePath(void)
{
    return EXAMPLE_TLS_CA_BUNDLE_PATH;
}

const char* ExampleTlsConfig_GetServerName(void)
{
    return ExampleTlsConfig_GetHost();
}

void ExampleTlsConfig_GetEndpoint(struct SolidSyslogEndpoint* endpoint)
{
    SolidSyslogFormatter_BoundedString(endpoint->host, ExampleTlsConfig_GetHost(), SOLIDSYSLOG_MAX_HOST_SIZE);
    endpoint->port = ExampleTlsConfig_GetPort();
}

/* Static config — host/port never change, so version stays 0 forever and the
   sender connects exactly once. */
uint32_t ExampleTlsConfig_GetEndpointVersion(void)
{
    return 0;
}

#include "ExampleMtlsConfig.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogFormatter.h"
#include "SolidSyslogEndpoint.h"

enum
{
    EXAMPLE_MTLS_PORT = 6515 /* S03.09 mTLS source on the BDD syslog-ng */
};

/* Test CA / client identity for BDD. Paths are relative to the working
   directory the example is launched from (/workspaces/SolidSyslog in the
   BDD container). */
static const char* const EXAMPLE_MTLS_CA_BUNDLE_PATH         = "Bdd/syslog-ng/tls/ca.pem";
static const char* const EXAMPLE_MTLS_CLIENT_CERT_CHAIN_PATH = "Bdd/syslog-ng/tls/client.pem";
static const char* const EXAMPLE_MTLS_CLIENT_KEY_PATH        = "Bdd/syslog-ng/tls/client.key";

static const char* mtlsHost = "syslog-ng";

void ExampleMtlsConfig_SetHost(const char* host)
{
    if (host != NULL)
    {
        mtlsHost = host;
    }
}

const char* ExampleMtlsConfig_GetHost(void)
{
    return mtlsHost;
}

uint16_t ExampleMtlsConfig_GetPort(void)
{
    return (uint16_t) EXAMPLE_MTLS_PORT;
}

const char* ExampleMtlsConfig_GetCaBundlePath(void)
{
    return EXAMPLE_MTLS_CA_BUNDLE_PATH;
}

const char* ExampleMtlsConfig_GetServerName(void)
{
    return ExampleMtlsConfig_GetHost();
}

const char* ExampleMtlsConfig_GetClientCertChainPath(void)
{
    return EXAMPLE_MTLS_CLIENT_CERT_CHAIN_PATH;
}

const char* ExampleMtlsConfig_GetClientKeyPath(void)
{
    return EXAMPLE_MTLS_CLIENT_KEY_PATH;
}

void ExampleMtlsConfig_GetEndpoint(struct SolidSyslogEndpoint* endpoint)
{
    SolidSyslogFormatter_BoundedString(endpoint->host, ExampleMtlsConfig_GetHost(), SOLIDSYSLOG_MAX_HOST_SIZE);
    endpoint->port = ExampleMtlsConfig_GetPort();
}

/* Static config — host/port never change, so version stays 0 forever and the
   sender connects exactly once. */
uint32_t ExampleMtlsConfig_GetEndpointVersion(void)
{
    return 0;
}

#include "BddTargetTlsConfig.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogFormatter.h"
#include "SolidSyslogTransport.h"
#include "SolidSyslogEndpoint.h"

/* Test CA for BDD. Paths are relative to the working directory the example is
 * launched from (/workspaces/SolidSyslog in the BDD container). */
static const char* const BDD_TARGET_TLS_CA_BUNDLE_PATH = "Bdd/syslog-ng/tls/ca.pem";

static const char* tlsHost = "syslog-ng";
static const char* tlsServerName = NULL;

void BddTargetTlsConfig_SetHost(const char* host)
{
    if (host != NULL)
    {
        tlsHost = host;
    }
}

void BddTargetTlsConfig_SetServerName(const char* serverName)
{
    if (serverName != NULL)
    {
        tlsServerName = serverName;
    }
}

const char* BddTargetTlsConfig_GetHost(void)
{
    return tlsHost;
}

uint16_t BddTargetTlsConfig_GetPort(void)
{
    return (uint16_t) SOLIDSYSLOG_TLS_DEFAULT_PORT;
}

const char* BddTargetTlsConfig_GetCaBundlePath(void)
{
    return BDD_TARGET_TLS_CA_BUNDLE_PATH;
}

const char* BddTargetTlsConfig_GetServerName(void)
{
    return (tlsServerName != NULL) ? tlsServerName : BddTargetTlsConfig_GetHost();
}

void BddTargetTlsConfig_GetEndpoint(struct SolidSyslogEndpoint* endpoint)
{
    SolidSyslogFormatter_BoundedString(endpoint->Host, BddTargetTlsConfig_GetHost(), SOLIDSYSLOG_MAX_HOST_SIZE);
    endpoint->Port = BddTargetTlsConfig_GetPort();
}

/* Static config — host/port never change, so version stays 0 forever and the
   sender connects exactly once. */
uint32_t BddTargetTlsConfig_GetEndpointVersion(void)
{
    return 0;
}

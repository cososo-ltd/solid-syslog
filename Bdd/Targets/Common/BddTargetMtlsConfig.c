#include "BddTargetMtlsConfig.h"

#include <stddef.h>
#include <stdint.h>

#include "SolidSyslogFormatter.h"
#include "SolidSyslogEndpoint.h"

enum
{
    BDD_TARGET_MTLS_PORT = 6515 /* S03.09 mTLS source on the BDD syslog-ng */
};

/* Test CA / client identity for BDD. Paths are relative to the working
   directory the example is launched from (/workspaces/SolidSyslog in the
   BDD container). */
static const char* const BDD_TARGET_MTLS_CA_BUNDLE_PATH = "Bdd/syslog-ng/tls/ca.pem";
static const char* const BDD_TARGET_MTLS_CLIENT_CERT_CHAIN_PATH = "Bdd/syslog-ng/tls/client.pem";
static const char* const BDD_TARGET_MTLS_CLIENT_KEY_PATH = "Bdd/syslog-ng/tls/client.key";

static const char* mtlsHost = "syslog-ng";

void BddTargetMtlsConfig_SetHost(const char* host)
{
    if (host != NULL)
    {
        mtlsHost = host;
    }
}

const char* BddTargetMtlsConfig_GetHost(void)
{
    return mtlsHost;
}

uint16_t BddTargetMtlsConfig_GetPort(void)
{
    return (uint16_t) BDD_TARGET_MTLS_PORT;
}

const char* BddTargetMtlsConfig_GetCaBundlePath(void)
{
    return BDD_TARGET_MTLS_CA_BUNDLE_PATH;
}

const char* BddTargetMtlsConfig_GetServerName(void)
{
    return BddTargetMtlsConfig_GetHost();
}

const char* BddTargetMtlsConfig_GetClientCertChainPath(void)
{
    return BDD_TARGET_MTLS_CLIENT_CERT_CHAIN_PATH;
}

const char* BddTargetMtlsConfig_GetClientKeyPath(void)
{
    return BDD_TARGET_MTLS_CLIENT_KEY_PATH;
}

void BddTargetMtlsConfig_GetEndpoint(struct SolidSyslogEndpoint* endpoint)
{
    SolidSyslogFormatter_BoundedString(endpoint->Host, BddTargetMtlsConfig_GetHost(), SOLIDSYSLOG_MAX_HOST_SIZE);
    endpoint->Port = BddTargetMtlsConfig_GetPort();
}

/* Static config — host/port never change, so version stays 0 forever and the
   sender connects exactly once. */
uint32_t BddTargetMtlsConfig_GetEndpointVersion(void)
{
    return 0;
}

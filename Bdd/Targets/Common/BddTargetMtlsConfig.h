#ifndef BDDTARGETMTLSCONFIG_H
#define BDDTARGETMTLSCONFIG_H

#include <stdint.h>

#include "ExternC.h"

struct SolidSyslogEndpoint;

EXTERN_C_BEGIN

    const char* BddTargetMtlsConfig_GetHost(void);
    uint16_t BddTargetMtlsConfig_GetPort(void);
    const char* BddTargetMtlsConfig_GetCaBundlePath(void);
    const char* BddTargetMtlsConfig_GetServerName(void);
    const char* BddTargetMtlsConfig_GetClientCertChainPath(void);
    const char* BddTargetMtlsConfig_GetClientKeyPath(void);
    void BddTargetMtlsConfig_GetEndpoint(struct SolidSyslogEndpoint * endpoint);
    uint32_t BddTargetMtlsConfig_GetEndpointVersion(void);

    /* Override the default mTLS host ("syslog-ng" — Linux compose service
       name). Caller owns the string lifetime. Used by per-platform main.c
       to inject SOLIDSYSLOG_BDD_MTLS_HOST when set. */
    void BddTargetMtlsConfig_SetHost(const char* host);

    /* Override the mTLS server name used for SNI and cert hostname
       verification, independently of the connection host. See the
       matching note in BddTargetTlsConfig.h — FreeRTOS BDD-on-QEMU
       needs the connection IP separate from the cert subject. */
    void BddTargetMtlsConfig_SetServerName(const char* serverName);

EXTERN_C_END

#endif /* BDDTARGETMTLSCONFIG_H */

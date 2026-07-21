#ifndef BDDTARGETTLSCONFIG_H
#define BDDTARGETTLSCONFIG_H

#include <stdint.h>

#include "ExternC.h"

struct SolidSyslogEndpoint;

EXTERN_C_BEGIN

    const char* BddTargetTlsConfig_GetHost(void);
    uint16_t BddTargetTlsConfig_GetPort(void);
    const char* BddTargetTlsConfig_GetCaBundlePath(void);
    const char* BddTargetTlsConfig_GetServerName(void);
    void BddTargetTlsConfig_GetEndpoint(struct SolidSyslogEndpoint * endpoint, void* context);
    uint32_t BddTargetTlsConfig_GetEndpointVersion(void* context);

    /* Override the default TLS host ("syslog-ng" — Linux compose service
       name). Caller owns the string lifetime. Used by the per-platform
       main.c to inject SOLIDSYSLOG_BDD_TLS_HOST when set, so the same
       example targets the Linux compose oracle or the Windows OTel oracle
       on 127.0.0.1. */
    void BddTargetTlsConfig_SetHost(const char* host);

    /* Override the TLS server name used for SNI and cert hostname
       verification, independently of the connection host. By default
       _GetServerName aliases _GetHost (the Linux / Windows BDD setup
       uses the cert subject as the connection host), but the FreeRTOS
       BDD target's QEMU networking needs the connection IP separate
       from the cert subject — slirp NAT goes through 10.0.2.2, while
       the syslog-ng oracle's cert is for "syslog-ng". Caller owns the
       string lifetime. */
    void BddTargetTlsConfig_SetServerName(const char* serverName);

EXTERN_C_END

#endif /* BDDTARGETTLSCONFIG_H */

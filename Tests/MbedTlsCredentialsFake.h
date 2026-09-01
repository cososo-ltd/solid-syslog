#ifndef MBEDTLSCREDENTIALSFAKE_H
#define MBEDTLSCREDENTIALSFAKE_H

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogExternC.h"

struct mbedtls_ssl_config;

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogMbedTlsCredentials;

    /* A credentials double for the MbedTlsStream tests: records the calls the
       stream makes, and lets a test dictate what Install reports back. Backed
       by a single static instance, so Reset is what separates one test from
       the next - there is nothing to destroy. */
    struct SolidSyslogMbedTlsCredentials* MbedTlsCredentialsFake_Get(void);
    void MbedTlsCredentialsFake_Reset(void);

    int MbedTlsCredentialsFake_InstallCallCount(void);
    struct mbedtls_ssl_config* MbedTlsCredentialsFake_LastInstallConfig(void);
    int MbedTlsCredentialsFake_ReleaseCallCount(void);

    /* Called from inside Release, so a test can observe what has already
       happened at the moment the credential window closes. */
    void MbedTlsCredentialsFake_SetReleaseObserver(void (*observer)(void));

    void MbedTlsCredentialsFake_SetInstallSucceeds(bool succeeds);
    void MbedTlsCredentialsFake_SetTrustAnchorsInstalled(bool installed);
    void MbedTlsCredentialsFake_SetFingerprints(const char* const * fingerprints, size_t count);

SOLIDSYSLOG_EXTERN_C_END

#endif /* MBEDTLSCREDENTIALSFAKE_H */

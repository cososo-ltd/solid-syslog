#ifndef OPENSSLCREDENTIALSFAKE_H
#define OPENSSLCREDENTIALSFAKE_H

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogExternC.h"

struct ssl_ctx_st;

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogOpenSslCredentials;

    /* A credentials double for the OpenSslStream tests: records the calls the
       stream makes, and lets a test dictate what Install reports back. Backed
       by a single static instance, so Reset is what separates one test from
       the next - there is nothing to destroy. */
    struct SolidSyslogOpenSslCredentials* OpenSslCredentialsFake_Get(void);
    void OpenSslCredentialsFake_Reset(void);

    int OpenSslCredentialsFake_InstallCallCount(void);
    struct ssl_ctx_st* OpenSslCredentialsFake_LastInstallCtx(void);
    int OpenSslCredentialsFake_ReleaseCallCount(void);

    void OpenSslCredentialsFake_SetInstallSucceeds(bool succeeds);
    void OpenSslCredentialsFake_SetTrustAnchorsInstalled(bool installed);
    void OpenSslCredentialsFake_SetFingerprints(const char* const * fingerprints, size_t count);

SOLIDSYSLOG_EXTERN_C_END

#endif /* OPENSSLCREDENTIALSFAKE_H */

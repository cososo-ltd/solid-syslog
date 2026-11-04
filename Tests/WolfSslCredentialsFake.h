#ifndef WOLFSSLCREDENTIALSFAKE_H
#define WOLFSSLCREDENTIALSFAKE_H

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogExternC.h"

struct WOLFSSL_CTX;

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogWolfSslCredentials;

    /* A credentials double for the WolfSslStream tests: records the calls the
       stream makes, and lets a test dictate what Install reports back. Backed
       by a single static instance, so Reset is what separates one test from
       the next - there is nothing to destroy. */
    struct SolidSyslogWolfSslCredentials* WolfSslCredentialsFake_Get(void);
    void WolfSslCredentialsFake_Reset(void);

    int WolfSslCredentialsFake_InstallCallCount(void);
    struct WOLFSSL_CTX* WolfSslCredentialsFake_LastInstallContext(void);
    int WolfSslCredentialsFake_ReleaseCallCount(void);

    /* Called from inside Install, before it reports back, so a test can observe
       what the stream had already done when the backend got its turn. */
    void WolfSslCredentialsFake_SetInstallObserver(void (*observer)(void));

    /* Called from inside Release, so a test can observe what has already
       happened at the moment the credential window closes. */
    void WolfSslCredentialsFake_SetReleaseObserver(void (*observer)(void));

    void WolfSslCredentialsFake_SetInstallSucceeds(bool succeeds);
    void WolfSslCredentialsFake_SetTrustAnchorsInstalled(bool installed);
    void WolfSslCredentialsFake_SetFingerprints(const char* const * fingerprints, size_t count);

SOLIDSYSLOG_EXTERN_C_END

#endif /* WOLFSSLCREDENTIALSFAKE_H */

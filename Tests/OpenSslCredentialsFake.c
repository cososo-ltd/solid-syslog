#include "OpenSslCredentialsFake.h"

#include <stddef.h>

#include "SolidSyslogOpenSslCredentialsDefinition.h"
#include "SolidSyslogTlsCredentialsInstalled.h"

struct OpenSslCredentialsFake
{
    struct SolidSyslogOpenSslCredentials Base;
    int InstallCallCount;
    struct ssl_ctx_st* LastInstallCtx;
    int ReleaseCallCount;
    bool InstallSucceeds;
    bool TrustAnchorsInstalled;
    const char* const * Fingerprints;
    size_t FingerprintCount;
};

static struct OpenSslCredentialsFake fake;

static bool Install(
    struct SolidSyslogOpenSslCredentials* self,
    SSL_CTX* ctx,
    struct SolidSyslogTlsCredentialsInstalled* installed
)
{
    (void) self;
    fake.InstallCallCount++;
    fake.LastInstallCtx = ctx;
    installed->TrustAnchorsInstalled = fake.TrustAnchorsInstalled;
    installed->Fingerprints = fake.Fingerprints;
    installed->FingerprintCount = fake.FingerprintCount;
    return fake.InstallSucceeds;
}

static void Release(struct SolidSyslogOpenSslCredentials* self)
{
    (void) self;
    fake.ReleaseCallCount++;
}

struct SolidSyslogOpenSslCredentials* OpenSslCredentialsFake_Get(void)
{
    return &fake.Base;
}

void OpenSslCredentialsFake_Reset(void)
{
    fake.Base.Install = Install;
    fake.Base.Release = Release;
    fake.InstallCallCount = 0;
    fake.LastInstallCtx = NULL;
    fake.ReleaseCallCount = 0;
    fake.InstallSucceeds = true;
    fake.TrustAnchorsInstalled = true;
    fake.Fingerprints = NULL;
    fake.FingerprintCount = 0U;
}

int OpenSslCredentialsFake_InstallCallCount(void)
{
    return fake.InstallCallCount;
}

struct ssl_ctx_st* OpenSslCredentialsFake_LastInstallCtx(void)
{
    return fake.LastInstallCtx;
}

int OpenSslCredentialsFake_ReleaseCallCount(void)
{
    return fake.ReleaseCallCount;
}

void OpenSslCredentialsFake_SetInstallSucceeds(bool succeeds)
{
    fake.InstallSucceeds = succeeds;
}

void OpenSslCredentialsFake_SetTrustAnchorsInstalled(bool installed)
{
    fake.TrustAnchorsInstalled = installed;
}

void OpenSslCredentialsFake_SetFingerprints(const char* const * fingerprints, size_t count)
{
    fake.Fingerprints = fingerprints;
    fake.FingerprintCount = count;
}

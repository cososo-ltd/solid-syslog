#include "WolfSslCredentialsFake.h"

#include <stddef.h>

#include "SolidSyslogTlsCredentialsInstalled.h"
#include "SolidSyslogWolfSslCredentialsDefinition.h"

struct WolfSslCredentialsFake
{
    struct SolidSyslogWolfSslCredentials Base;
    int InstallCallCount;
    struct WOLFSSL_CTX* LastInstallContext;
    int ReleaseCallCount;
    bool InstallSucceeds;
    bool TrustAnchorsInstalled;
    const char* const * Fingerprints;
    size_t FingerprintCount;
    void (*InstallObserver)(void);
    void (*ReleaseObserver)(void);
};

static struct WolfSslCredentialsFake fake;

static bool Install(
    struct SolidSyslogWolfSslCredentials* self,
    struct WOLFSSL_CTX* ctx,
    struct SolidSyslogTlsCredentialsInstalled* installed
)
{
    (void) self;
    fake.InstallCallCount++;
    fake.LastInstallContext = ctx;
    if (fake.InstallObserver != NULL)
    {
        fake.InstallObserver();
    }
    installed->TrustAnchorsInstalled = fake.TrustAnchorsInstalled;
    installed->Fingerprints = fake.Fingerprints;
    installed->FingerprintCount = fake.FingerprintCount;
    return fake.InstallSucceeds;
}

static void Release(struct SolidSyslogWolfSslCredentials* self)
{
    (void) self;
    fake.ReleaseCallCount++;
    if (fake.ReleaseObserver != NULL)
    {
        fake.ReleaseObserver();
    }
}

struct SolidSyslogWolfSslCredentials* WolfSslCredentialsFake_Get(void)
{
    return &fake.Base;
}

void WolfSslCredentialsFake_Reset(void)
{
    fake.Base.Install = Install;
    fake.Base.Release = Release;
    fake.InstallCallCount = 0;
    fake.LastInstallContext = NULL;
    fake.ReleaseCallCount = 0;
    fake.InstallSucceeds = true;
    fake.TrustAnchorsInstalled = true;
    fake.Fingerprints = NULL;
    fake.FingerprintCount = 0U;
    fake.InstallObserver = NULL;
    fake.ReleaseObserver = NULL;
}

int WolfSslCredentialsFake_InstallCallCount(void)
{
    return fake.InstallCallCount;
}

struct WOLFSSL_CTX* WolfSslCredentialsFake_LastInstallContext(void)
{
    return fake.LastInstallContext;
}

int WolfSslCredentialsFake_ReleaseCallCount(void)
{
    return fake.ReleaseCallCount;
}

void WolfSslCredentialsFake_SetInstallObserver(void (*observer)(void))
{
    fake.InstallObserver = observer;
}

void WolfSslCredentialsFake_SetReleaseObserver(void (*observer)(void))
{
    fake.ReleaseObserver = observer;
}

void WolfSslCredentialsFake_SetInstallSucceeds(bool succeeds)
{
    fake.InstallSucceeds = succeeds;
}

void WolfSslCredentialsFake_SetTrustAnchorsInstalled(bool installed)
{
    fake.TrustAnchorsInstalled = installed;
}

void WolfSslCredentialsFake_SetFingerprints(const char* const * fingerprints, size_t count)
{
    fake.Fingerprints = fingerprints;
    fake.FingerprintCount = count;
}

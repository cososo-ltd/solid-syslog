#include "MbedTlsCredentialsFake.h"

#include <stddef.h>

#include "SolidSyslogMbedTlsCredentialsDefinition.h"
#include "SolidSyslogTlsCredentialsInstalled.h"

struct MbedTlsCredentialsFake
{
    struct SolidSyslogMbedTlsCredentials Base;
    int InstallCallCount;
    struct mbedtls_ssl_config* LastInstallConfig;
    int ReleaseCallCount;
    bool InstallSucceeds;
    bool TrustAnchorsInstalled;
    const char* const * Fingerprints;
    size_t FingerprintCount;
    void (*ReleaseObserver)(void);
};

static struct MbedTlsCredentialsFake fake;

static bool Install(
    struct SolidSyslogMbedTlsCredentials* self,
    struct mbedtls_ssl_config* conf,
    struct SolidSyslogTlsCredentialsInstalled* installed
)
{
    (void) self;
    fake.InstallCallCount++;
    fake.LastInstallConfig = conf;
    installed->TrustAnchorsInstalled = fake.TrustAnchorsInstalled;
    installed->Fingerprints = fake.Fingerprints;
    installed->FingerprintCount = fake.FingerprintCount;
    return fake.InstallSucceeds;
}

static void Release(struct SolidSyslogMbedTlsCredentials* self)
{
    (void) self;
    fake.ReleaseCallCount++;
    if (fake.ReleaseObserver != NULL)
    {
        fake.ReleaseObserver();
    }
}

struct SolidSyslogMbedTlsCredentials* MbedTlsCredentialsFake_Get(void)
{
    return &fake.Base;
}

void MbedTlsCredentialsFake_Reset(void)
{
    fake.Base.Install = Install;
    fake.Base.Release = Release;
    fake.InstallCallCount = 0;
    fake.LastInstallConfig = NULL;
    fake.ReleaseCallCount = 0;
    fake.InstallSucceeds = true;
    fake.TrustAnchorsInstalled = true;
    fake.Fingerprints = NULL;
    fake.FingerprintCount = 0U;
    fake.ReleaseObserver = NULL;
}

int MbedTlsCredentialsFake_InstallCallCount(void)
{
    return fake.InstallCallCount;
}

struct mbedtls_ssl_config* MbedTlsCredentialsFake_LastInstallConfig(void)
{
    return fake.LastInstallConfig;
}

int MbedTlsCredentialsFake_ReleaseCallCount(void)
{
    return fake.ReleaseCallCount;
}

void MbedTlsCredentialsFake_SetReleaseObserver(void (*observer)(void))
{
    fake.ReleaseObserver = observer;
}

void MbedTlsCredentialsFake_SetInstallSucceeds(bool succeeds)
{
    fake.InstallSucceeds = succeeds;
}

void MbedTlsCredentialsFake_SetTrustAnchorsInstalled(bool installed)
{
    fake.TrustAnchorsInstalled = installed;
}

void MbedTlsCredentialsFake_SetFingerprints(const char* const * fingerprints, size_t count)
{
    fake.Fingerprints = fingerprints;
    fake.FingerprintCount = count;
}

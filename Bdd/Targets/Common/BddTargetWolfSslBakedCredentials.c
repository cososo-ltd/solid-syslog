/* The BDD target's wolfSSL credentials on a target with no filesystem: the same
   role BddTargetWolfSslCredentials.c fills from paths, filled from the PEMs
   baked into the image. wolfSSL takes a buffer with its length, so the arrays
   are handed over as xxd wrote them. */
#include "BddTargetWolfSslCredentials.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <wolfssl/wolfcrypt/settings.h>
#include <wolfssl/ssl.h>

#include "BddBakedCaBPem.h"
#include "BddBakedCaPem.h"
#include "BddBakedClientCertPem.h"
#include "BddBakedClientKeyPem.h"
#include "BddTargetSwitchConfig.h"
#include "BddTargetTlsConfig.h"
#include "SolidSyslogTlsCredentialsInstalled.h"
#include "SolidSyslogWolfSslCredentialsDefinition.h"

static bool BddTargetWolfSslCredentials_Install(
    struct SolidSyslogWolfSslCredentials* self,
    struct WOLFSSL_CTX* ctx,
    struct SolidSyslogTlsCredentialsInstalled* installed
);
static void BddTargetWolfSslCredentials_Release(struct SolidSyslogWolfSslCredentials* self);
static bool BddTargetWolfSslCredentials_InstallTrustAnchors(WOLFSSL_CTX* ctx, bool* installed);
static void BddTargetWolfSslCredentials_InstallClientCredential(WOLFSSL_CTX* ctx);
static void BddTargetWolfSslCredentials_Complain(const char* what);

static struct SolidSyslogWolfSslCredentials credentials = {
    BddTargetWolfSslCredentials_Install,
    BddTargetWolfSslCredentials_Release
};

struct SolidSyslogWolfSslCredentials* BddTargetWolfSslCredentials_Get(void)
{
    return &credentials;
}

/* Asked once per connection, so everything a scenario set at the prompt is read
   here rather than remembered from startup. */
static bool BddTargetWolfSslCredentials_Install(
    struct SolidSyslogWolfSslCredentials* self,
    struct WOLFSSL_CTX* ctx,
    struct SolidSyslogTlsCredentialsInstalled* installed
)
{
    (void) self;
    installed->Fingerprints = BddTargetTlsConfig_GetPeerFingerprints();
    installed->FingerprintCount = BddTargetTlsConfig_GetPeerFingerprintCount();

    bool ok = BddTargetWolfSslCredentials_InstallTrustAnchors(ctx, &installed->TrustAnchorsInstalled);
    BddTargetWolfSslCredentials_InstallClientCredential(ctx);
    return ok;
}

/* wolfSSL copies what it loads into the context the stream frees, so there is
   nothing of ours to hand back. */
static void BddTargetWolfSslCredentials_Release(struct SolidSyslogWolfSslCredentials* self)
{
    (void) self;
}

static bool BddTargetWolfSslCredentials_InstallTrustAnchors(WOLFSSL_CTX* ctx, bool* installed)
{
    const char* name = BddTargetTlsConfig_GetTrustAnchorName();
    const unsigned char* pem = NULL;
    long length = 0;
    if (strcmp(name, "ca") == 0)
    {
        pem = bdd_baked_ca_pem;
        length = (long) sizeof(bdd_baked_ca_pem);
    }
    else if (strcmp(name, "ca-b") == 0)
    {
        pem = bdd_baked_ca_b_pem;
        length = (long) sizeof(bdd_baked_ca_b_pem);
    }
    else
    {
        /* "none" - the configuration names no anchors, which the stream refuses
           unless a pin names the peer instead. */
    }

    bool ok = true;
    *installed = false;
    if (pem != NULL)
    {
        *installed = wolfSSL_CTX_load_verify_buffer(ctx, pem, length, WOLFSSL_FILETYPE_PEM) == WOLFSSL_SUCCESS;
        ok = *installed;
    }
    return ok;
}

/* Decided per connection, not at Create: this target is built with mtls false
   and switches transport at runtime, so a startup decision would never see
   mutual TLS selected. The mutual-TLS transport presents a credential by
   construction; the plain TLS transport presents whatever the knob says.

   A fault in our own credential never stops delivery - that is the contract the
   matrix is here to demonstrate - so nothing below fails the connection. It
   does complain when material the scenario asked for will not load: that is
   broken harness material, not a condition under test, and silence would let a
   mutual-TLS scenario pass with no client certificate at all. */
static void BddTargetWolfSslCredentials_InstallClientCredential(WOLFSSL_CTX* ctx)
{
    const char* wanted = BddTargetTlsConfig_GetClientCredentialName();
    bool wantsKey = BddTargetSwitchConfig_IsMtlsMode() || (strcmp(wanted, "client") == 0);
    bool wantsCertificate = wantsKey || (strcmp(wanted, "cert-only") == 0);

    if (wantsCertificate && (wolfSSL_CTX_use_certificate_chain_buffer(
                                 ctx,
                                 bdd_baked_client_cert_pem,
                                 (long) sizeof(bdd_baked_client_cert_pem)
                             ) != WOLFSSL_SUCCESS))
    {
        BddTargetWolfSslCredentials_Complain("client certificate");
        wantsKey = false;
    }
    if (wantsKey && (wolfSSL_CTX_use_PrivateKey_buffer(
                         ctx,
                         bdd_baked_client_key_pem,
                         (long) sizeof(bdd_baked_client_key_pem),
                         WOLFSSL_FILETYPE_PEM
                     ) != WOLFSSL_SUCCESS))
    {
        BddTargetWolfSslCredentials_Complain("client key");
    }
    /* "cert-only" stops here on purpose: half a credential is the case the
       contract reports and keeps delivering through. */
}

static void BddTargetWolfSslCredentials_Complain(const char* what)
{
    (void) printf("BDD-TARGET: harness material broken - no %s\r\n", what);
}

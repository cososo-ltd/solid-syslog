#include "BddTargetWolfSslCredentials.h"

#ifndef WOLFSSL_USER_SETTINGS
#include <wolfssl/options.h>
#endif
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <wolfssl/ssl.h>

#include "BddTargetTlsConfig.h"
#include "SolidSyslogTlsCredentialsInstalled.h"
#include "SolidSyslogWolfSslCredentialsDefinition.h"

/* Where each logical name lives on a target with a filesystem. The names
   themselves are the harness vocabulary; only this file knows they are paths. */
static const char* const TRUST_ANCHOR_CA = "Bdd/syslog-ng/tls/ca.pem";
static const char* const TRUST_ANCHOR_CA_B = "Bdd/syslog-ng/tls/ca-b.pem";
static const char* const CLIENT_CERT_CHAIN = "Bdd/syslog-ng/tls/client.pem";
static const char* const CLIENT_KEY = "Bdd/syslog-ng/tls/client.key";

static bool BddTargetWolfSslCredentials_Install(
    struct SolidSyslogWolfSslCredentials* self,
    struct WOLFSSL_CTX* ctx,
    struct SolidSyslogTlsCredentialsInstalled* installed
);
static void BddTargetWolfSslCredentials_Release(struct SolidSyslogWolfSslCredentials* self);
static const char* BddTargetWolfSslCredentials_TrustAnchorPath(void);
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
    /* No pins reach the stream until it enforces them: the pin cells are
       outside this lane, and a pin a scenario did set must not be quietly
       dropped on the floor by a stream that would not check it. */
    installed->Fingerprints = NULL;
    installed->FingerprintCount = 0U;
    installed->TrustAnchorsInstalled = false;

    bool ok = true;
    const char* anchors = BddTargetWolfSslCredentials_TrustAnchorPath();
    if (anchors != NULL)
    {
        installed->TrustAnchorsInstalled = wolfSSL_CTX_load_verify_locations(ctx, anchors, NULL) == WOLFSSL_SUCCESS;
        ok = installed->TrustAnchorsInstalled;
    }

    BddTargetWolfSslCredentials_InstallClientCredential(ctx);
    return ok;
}

/* Nothing is held between connections: the anchors are named by path and
   wolfSSL reads them into the context the stream frees, so there is nothing of
   ours to hand back. */
static void BddTargetWolfSslCredentials_Release(struct SolidSyslogWolfSslCredentials* self)
{
    (void) self;
}

static const char* BddTargetWolfSslCredentials_TrustAnchorPath(void)
{
    const char* name = BddTargetTlsConfig_GetTrustAnchorName();
    const char* path = NULL;
    if (strcmp(name, "ca") == 0)
    {
        path = TRUST_ANCHOR_CA;
    }
    else if (strcmp(name, "ca-b") == 0)
    {
        path = TRUST_ANCHOR_CA_B;
    }
    else
    {
        /* "none" - the configuration names no anchors, which the stream refuses
           unless a pin names the peer instead. */
    }
    return path;
}

/* A fault in our own credential never stops delivery - that is the contract the
   matrix is here to demonstrate - so nothing below fails the connection.

   What it does do is complain when material the scenario asked for will not
   load. That is broken harness material rather than a condition under test, and
   silence would let the mutual-TLS scenario connect with no client certificate
   at all and pass against a listener that does not require one. A green test
   proving the opposite of its name is the worst failure this suite can have.

   The complaint is deliberately not a SolidSyslog error report: it carries no
   role or detail, so it cannot be mistaken for something the library said. */
static void BddTargetWolfSslCredentials_InstallClientCredential(WOLFSSL_CTX* ctx)
{
    const char* wanted = BddTargetTlsConfig_GetClientCredentialName();
    bool wantsCertificate = (strcmp(wanted, "client") == 0) || (strcmp(wanted, "cert-only") == 0);
    bool wantsKey = strcmp(wanted, "client") == 0;

    if (wantsCertificate && (wolfSSL_CTX_use_certificate_chain_file(ctx, CLIENT_CERT_CHAIN) != WOLFSSL_SUCCESS))
    {
        BddTargetWolfSslCredentials_Complain("client certificate");
        wantsKey = false;
    }
    if (wantsKey && (wolfSSL_CTX_use_PrivateKey_file(ctx, CLIENT_KEY, WOLFSSL_FILETYPE_PEM) != WOLFSSL_SUCCESS))
    {
        BddTargetWolfSslCredentials_Complain("client key");
    }
    /* "cert-only" stops here on purpose: half a credential is the case the
       contract reports and keeps delivering through. */
}

static void BddTargetWolfSslCredentials_Complain(const char* what)
{
    (void) fprintf(stderr, "BDD-TARGET: harness material broken - no %s\n", what);
    (void) fflush(stderr);
}

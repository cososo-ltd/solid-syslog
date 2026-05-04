#ifndef TLSTESTSERVER_H
#define TLSTESTSERVER_H

#include <openssl/types.h>

#include "ExternC.h"

EXTERN_C_BEGIN

    struct TlsTestServer;

    struct TlsTestServerConfig
    {
        const struct TlsTestCert* serverCert;   /* includes matching private key */
        const char*               cipherList;   /* NULL = server default */
        const struct TlsTestCert* clientCaCert; /* NULL = no mTLS; set to require & verify client cert */
    };

    struct TlsTestServer* TlsTestServer_Create(const struct TlsTestServerConfig* config);
    void                  TlsTestServer_Destroy(struct TlsTestServer * self);

    /* Returns the client-facing end of the internal BIO pair. Pass to
     * BioPairStream_Create and use as the transport for SolidSyslogTlsStream. */
    BIO* TlsTestServer_ClientSideBio(struct TlsTestServer * self);

    /* Advances the server's state machine one step. Used as a pump callback
     * so client-side reads can make cooperative progress. Context is the
     * TlsTestServer pointer. */
    void TlsTestServer_Pump(void* context);

EXTERN_C_END

#endif /* TLSTESTSERVER_H */

#ifndef WOLFSSLTESTSERVER_H
#define WOLFSSLTESTSERVER_H

#include <stdbool.h>
#include <stddef.h>

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct WolfSslTestServer;

    struct WolfSslTestServerConfig
    {
        int ServerFd; /* one end of a socketpair; ownership transferred to the server */
        const char* CertificatePath; /* PEM leaf, or leaf then issuers */
        const char* KeyPath; /* PEM key matching the leaf */
    };

    /* Spawns a worker thread that drives the server-side handshake and then
     * reads one record. The server owns ServerFd from this point on. */
    struct WolfSslTestServer* WolfSslTestServer_Create(const struct WolfSslTestServerConfig* config);

    /* Joins the worker thread and tears down all wolfSSL state. */
    void WolfSslTestServer_Destroy(struct WolfSslTestServer * self);

    /* Wait for the worker thread to exit and return whether the handshake
     * completed successfully. */
    bool WolfSslTestServer_JoinAndHandshakeSucceeded(struct WolfSslTestServer * self);

    /* What the one read after the handshake delivered; joins first. Empty
     * where the client sent nothing before closing. */
    const char* WolfSslTestServer_Received(struct WolfSslTestServer * self);

SOLIDSYSLOG_EXTERN_C_END

#endif /* WOLFSSLTESTSERVER_H */

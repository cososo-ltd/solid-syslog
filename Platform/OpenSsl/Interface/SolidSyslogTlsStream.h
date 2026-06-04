#ifndef SOLIDSYSLOGTLSSTREAM_H
#define SOLIDSYSLOGTLSSTREAM_H

#include "ExternC.h"
#include "SolidSyslogSleep.h"
#include "SolidSyslogTlsHandshakeTimeoutFunction.h"

struct SolidSyslogStream;

EXTERN_C_BEGIN

    struct SolidSyslogTlsStreamConfig
    {
        struct SolidSyslogStream* Transport; /* underlying byte stream — caller owns */
        SolidSyslogSleepFunction
            Sleep; /* drives bounded handshake retry between WANT_READ/WANT_WRITE polls — required */
        SolidSyslogTlsHandshakeTimeoutFunction
            GetHandshakeTimeoutMs; /* NULL → use SOLIDSYSLOG_TLS_HANDSHAKE_TIMEOUT_MS tunable */
        void* HandshakeTimeoutContext; /* passed through to GetHandshakeTimeoutMs; NULL is fine */
        const char* CaBundlePath; /* PEM file of trust anchors */
        const char* ServerName; /* SNI + peer-identity check. A non-empty name is verified against the
                                   cert (SAN/CN). NULL connects chain-only but emits a WARNING — the peer
                                   is unverified (MITM-class). "" is the deliberate opt-out (IP-pinning /
                                   private CA): connect chain-only, no diagnostic. */
        const char* CipherList; /* TLS 1.2 cipher list; NULL = OpenSSL default */
        const char* ClientCertChainPath; /* PEM: leaf cert (+ intermediates); NULL = no mTLS */
        const char* ClientKeyPath; /* PEM: matching private key; NULL = no mTLS */
    };

    struct SolidSyslogStream* SolidSyslogTlsStream_Create(const struct SolidSyslogTlsStreamConfig* config);
    void SolidSyslogTlsStream_Destroy(struct SolidSyslogStream * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGTLSSTREAM_H */

#ifndef SOLIDSYSLOGTLSSTREAM_H
#define SOLIDSYSLOGTLSSTREAM_H

#include <stdint.h>

#include "ExternC.h"
#include "SolidSyslogSleep.h"

struct SolidSyslogStream;

EXTERN_C_BEGIN

    enum
    {
        SOLIDSYSLOG_TLS_STREAM_SIZE = sizeof(intptr_t) * 14U
    };

    typedef struct
    {
        intptr_t slots[(SOLIDSYSLOG_TLS_STREAM_SIZE + sizeof(intptr_t) - 1U) / sizeof(intptr_t)];
    } SolidSyslogTlsStreamStorage;

    struct SolidSyslogTlsStreamConfig
    {
        struct SolidSyslogStream* Transport; /* underlying byte stream — caller owns */
        SolidSyslogSleepFunction
            Sleep; /* drives bounded handshake retry between WANT_READ/WANT_WRITE polls — required */
        const char* CaBundlePath; /* PEM file of trust anchors */
        const char* ServerName; /* SNI + cert hostname check; NULL to skip */
        const char* CipherList; /* TLS 1.2 cipher list; NULL = OpenSSL default */
        const char* ClientCertChainPath; /* PEM: leaf cert (+ intermediates); NULL = no mTLS */
        const char* ClientKeyPath; /* PEM: matching private key; NULL = no mTLS */
    };

    struct SolidSyslogStream* SolidSyslogTlsStream_Create(
        SolidSyslogTlsStreamStorage * storage,
        const struct SolidSyslogTlsStreamConfig* config
    );
    void SolidSyslogTlsStream_Destroy(struct SolidSyslogStream * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGTLSSTREAM_H */

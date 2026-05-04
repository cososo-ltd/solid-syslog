#ifndef SOLIDSYSLOGTLSSTREAM_H
#define SOLIDSYSLOGTLSSTREAM_H

#include <stdint.h>

#include "ExternC.h"

struct SolidSyslogStream;

EXTERN_C_BEGIN

    enum
    {
        SOLIDSYSLOG_TLS_STREAM_SIZE = sizeof(intptr_t) * 13
    };

    typedef struct
    {
        intptr_t slots[(SOLIDSYSLOG_TLS_STREAM_SIZE + sizeof(intptr_t) - 1) / sizeof(intptr_t)];
    } SolidSyslogTlsStreamStorage;

    struct SolidSyslogTlsStreamConfig
    {
        struct SolidSyslogStream* transport;           /* underlying byte stream — caller owns */
        const char*               caBundlePath;        /* PEM file of trust anchors */
        const char*               serverName;          /* SNI + cert hostname check; NULL to skip */
        const char*               cipherList;          /* TLS 1.2 cipher list; NULL = OpenSSL default */
        const char*               clientCertChainPath; /* PEM: leaf cert (+ intermediates); NULL = no mTLS */
        const char*               clientKeyPath;       /* PEM: matching private key; NULL = no mTLS */
    };

    struct SolidSyslogStream* SolidSyslogTlsStream_Create(SolidSyslogTlsStreamStorage * storage, const struct SolidSyslogTlsStreamConfig* config);
    void                      SolidSyslogTlsStream_Destroy(struct SolidSyslogStream * stream);

EXTERN_C_END

#endif /* SOLIDSYSLOGTLSSTREAM_H */

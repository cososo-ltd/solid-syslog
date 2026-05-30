#ifndef SOLIDSYSLOGMBEDTLSSTREAMPRIVATE_H
#define SOLIDSYSLOGMBEDTLSSTREAMPRIVATE_H

#include <mbedtls/ssl.h>
#include <stdbool.h>

#include "SolidSyslogMbedTlsStream.h"
#include "SolidSyslogStreamDefinition.h"

struct SolidSyslogMbedTlsStream
{
    struct SolidSyslogStream Base;
    struct SolidSyslogMbedTlsStreamConfig Config;
    mbedtls_ssl_config SslConfig;
    mbedtls_ssl_context SslContext;
    mbedtls_ssl_session SavedSession; /* resumption: captured after handshake, fed back on next Open */
    bool HasSavedSession;
};

void MbedTlsStream_Initialise(struct SolidSyslogStream* base, const struct SolidSyslogMbedTlsStreamConfig* config);
void MbedTlsStream_Cleanup(struct SolidSyslogStream* base);

#endif /* SOLIDSYSLOGMBEDTLSSTREAMPRIVATE_H */

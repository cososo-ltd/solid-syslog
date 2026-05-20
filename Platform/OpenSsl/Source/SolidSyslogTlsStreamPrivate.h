#ifndef SOLIDSYSLOGTLSSTREAMPRIVATE_H
#define SOLIDSYSLOGTLSSTREAMPRIVATE_H

#include <openssl/bio.h>
#include <openssl/types.h>

#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogTlsStream.h"

struct SolidSyslogTlsStream
{
    struct SolidSyslogStream Base;
    struct SolidSyslogTlsStreamConfig Config;
    SSL_CTX* Ctx;
    SSL* Ssl;
    BIO_METHOD* BioMethod;
};

void TlsStream_Initialise(struct SolidSyslogStream* base, const struct SolidSyslogTlsStreamConfig* config);
void TlsStream_Cleanup(struct SolidSyslogStream* base);

#endif /* SOLIDSYSLOGTLSSTREAMPRIVATE_H */

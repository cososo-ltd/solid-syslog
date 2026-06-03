#ifndef SOLIDSYSLOGTLSSTREAMPRIVATE_H
#define SOLIDSYSLOGTLSSTREAMPRIVATE_H

#include <stdint.h>

#include <openssl/bio.h>
#include <openssl/types.h>

#include "SolidSyslogError.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogStreamDefinition.h"
#include "SolidSyslogTlsStream.h"
#include "SolidSyslogTlsStreamErrors.h"

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

static inline void TlsStream_Report(
    enum SolidSyslogSeverity severity,
    uint16_t category,
    enum SolidSyslogTlsStreamErrors code
)
{
    SolidSyslog_Error(severity, &TlsStreamErrorSource, category, code);
}

#endif /* SOLIDSYSLOGTLSSTREAMPRIVATE_H */

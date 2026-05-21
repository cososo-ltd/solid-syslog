#ifndef SOLIDSYSLOGMBEDTLSSTREAMPRIVATE_H
#define SOLIDSYSLOGMBEDTLSSTREAMPRIVATE_H

#include "SolidSyslogMbedTlsStream.h"
#include "SolidSyslogStreamDefinition.h"

struct SolidSyslogMbedTlsStream
{
    struct SolidSyslogStream Base;
    struct SolidSyslogMbedTlsStreamConfig Config;
};

void MbedTlsStream_Initialise(struct SolidSyslogStream* base, const struct SolidSyslogMbedTlsStreamConfig* config);
void MbedTlsStream_Cleanup(struct SolidSyslogStream* base);

#endif /* SOLIDSYSLOGMBEDTLSSTREAMPRIVATE_H */

#ifndef BDDTARGETTLSINNERSTREAM_H
#define BDDTARGETTLSINNERSTREAM_H

#include "SolidSyslogExternC.h"

SOLIDSYSLOG_EXTERN_C_BEGIN

    struct SolidSyslogAddress;
    struct SolidSyslogStream;

    /* The byte transport a TLS stream runs over, and the address handle the
     * StreamSender reads its resolved destination back into. One file per
     * network pack supplies these; they are the only thing that differs between
     * one mbedTLS BDD target and another, which is why BddTargetTlsSender_MbedTls.c
     * is shared and these are not. */
    struct SolidSyslogStream* BddTargetTlsInnerStream_CreateStream(void);
    void BddTargetTlsInnerStream_DestroyStream(struct SolidSyslogStream * stream);

    struct SolidSyslogAddress* BddTargetTlsInnerStream_CreateAddress(void);
    void BddTargetTlsInnerStream_DestroyAddress(struct SolidSyslogAddress * address);

SOLIDSYSLOG_EXTERN_C_END

#endif /* BDDTARGETTLSINNERSTREAM_H */

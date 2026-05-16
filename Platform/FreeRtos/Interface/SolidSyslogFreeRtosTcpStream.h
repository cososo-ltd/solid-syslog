#ifndef SOLIDSYSLOGFREERTOSTCPSTREAM_H
#define SOLIDSYSLOGFREERTOSTCPSTREAM_H

#include "ExternC.h"

#include <stdint.h>

EXTERN_C_BEGIN

    struct SolidSyslogStream;

    enum
    {
        SOLIDSYSLOG_FREERTOSTCPSTREAM_SIZE = sizeof(intptr_t) * 8U
    };

    typedef struct
    {
        intptr_t slots[(SOLIDSYSLOG_FREERTOSTCPSTREAM_SIZE + sizeof(intptr_t) - 1U) / sizeof(intptr_t)];
    } SolidSyslogFreeRtosTcpStreamStorage;

    struct SolidSyslogStream* SolidSyslogFreeRtosTcpStream_Create(SolidSyslogFreeRtosTcpStreamStorage * storage);
    void SolidSyslogFreeRtosTcpStream_Destroy(struct SolidSyslogStream * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGFREERTOSTCPSTREAM_H */

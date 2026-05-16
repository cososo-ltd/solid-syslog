#ifndef SOLIDSYSLOGPOSIXTCPSTREAM_H
#define SOLIDSYSLOGPOSIXTCPSTREAM_H

#include <stdint.h>

#include "ExternC.h"

struct SolidSyslogStream;

EXTERN_C_BEGIN

    enum
    {
        SOLIDSYSLOG_POSIX_TCP_STREAM_SIZE = sizeof(intptr_t) * 5U
    };

    typedef struct
    {
        intptr_t slots[(SOLIDSYSLOG_POSIX_TCP_STREAM_SIZE + sizeof(intptr_t) - 1U) / sizeof(intptr_t)];
    } SolidSyslogPosixTcpStreamStorage;

    struct SolidSyslogStream* SolidSyslogPosixTcpStream_Create(SolidSyslogPosixTcpStreamStorage * storage);
    void SolidSyslogPosixTcpStream_Destroy(struct SolidSyslogStream * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXTCPSTREAM_H */

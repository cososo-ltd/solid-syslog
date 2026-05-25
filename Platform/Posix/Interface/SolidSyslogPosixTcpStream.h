#ifndef SOLIDSYSLOGPOSIXTCPSTREAM_H
#define SOLIDSYSLOGPOSIXTCPSTREAM_H

#include "ExternC.h"
#include "SolidSyslogTcpConnectTimeoutFunction.h"

EXTERN_C_BEGIN

    struct SolidSyslogStream;

    struct SolidSyslogPosixTcpStreamConfig
    {
        SolidSyslogTcpConnectTimeoutFunction
            GetConnectTimeoutMs; /* NULL → use SOLIDSYSLOG_TCP_CONNECT_TIMEOUT_MS tunable */
        void* ConnectTimeoutContext; /* passed through to GetConnectTimeoutMs; NULL is fine */
    };

    struct SolidSyslogStream* SolidSyslogPosixTcpStream_Create(const struct SolidSyslogPosixTcpStreamConfig* config);
    void SolidSyslogPosixTcpStream_Destroy(struct SolidSyslogStream * base);

EXTERN_C_END

#endif /* SOLIDSYSLOGPOSIXTCPSTREAM_H */
